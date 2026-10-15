#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <stdio.h> // printf, fgets, perror
#include <stdlib.h> // realloc, free, strtol
#include <string.h> // strchr
#include <unistd.h> // read, lseek, close
#include <fcntl.h> // open, O_RDONLY
#include <errno.h> // errno, EINTR
#include <signal.h> // sigaction, SIGALRM, sig_atomic_t

struct Line
{
    off_t offset; // начало строки в файле
    size_t length; // длина вместе с переводом строки
};

static volatile sig_atomic_t timed_out = 0; // флаг изменяет обработчик сигнала

static void timeout_handler(int signal_number) // обработчик сигнала окончания пяти секунд
{
    (void)signal_number;
    timed_out = 1; // ввод/вывод внутри обработчика не выполняем
}

static int make_table(int fd, struct Line **table, size_t *count) // строим таблицу смещений и длин строк
{
    struct Line *next; // новый адрес таблицы после realloc
    off_t offset; // смещение от начала файла в байтах
    size_t length; // длина текущей строки в байтах
    char ch; // очередной символ файла
    ssize_t result; // число прочитанных байт или -1 при ошибке

    while (1)
    {
        offset = lseek(fd, 0L, SEEK_CUR); // текущая позиция станет началом строки
        if (offset == (off_t)-1)
        {
            perror("lseek");
            return -1;
        }
        length = 0; // начинаем подсчёт длины новой строки
        while (1)
        {
            result = read(fd, &ch, 1); // ищем перевод строки
            if (result == -1 && errno == EINTR) // повторяем чтение, прерванное сигналом
            {
                continue;
            }
            if (result == -1)
            {
                perror("read");
                return -1;
            }
            if (result == 0) // достигнут конец файла
            {
                break;
            }
            length++; // учитываем прочитанный символ
            if (ch == '\n')
            {
                break;
            }
        }

        if (length == 0) // после конца файла новую строку не добавляем
        {
            break;
        }
        next = realloc(*table, (*count + 1) * sizeof(**table)); // расширяем таблицу на одну запись
        if (next == NULL)
        {
            perror("realloc");
            return -1;
        }
        *table = next; // сохраняем новый адрес выделенной памяти
        (*table)[*count].offset = offset; // запоминаем начало строки
        (*table)[*count].length = length; // запоминаем точное количество байт строки
        (*count)++; // увеличиваем количество записей
        if (result == 0) // последняя строка может не иметь перевода строки
        {
            break;
        }
    }
    return 0;
}

static int print_text(int fd, off_t offset, size_t length) // читаем и выводим заданный диапазон файла
{
    char buffer[1025]; // 1024 байта текста и место для завершающего нуля
    size_t portion; // размер очередной порции вывода
    ssize_t result; // число прочитанных байт или -1 при ошибке

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) // переходим к началу выбранного диапазона
    {
        perror("lseek");
        return -1;
    }
    while (length > 0)
    {
        portion = length < sizeof(buffer) - 1 ? length : sizeof(buffer) - 1; // берём остаток строки или полную порцию
        result = read(fd, buffer, portion); // читаем только длину выбранной строки
        if (result == -1 && errno == EINTR) // повторяем чтение, прерванное сигналом
        {
            continue;
        }
        if (result == -1)
        {
            perror("read");
            return -1;
        }
        if (result == 0)
        {
            fprintf(stderr, "File ended before the indexed line was read\n");
            return -1;
        }
        buffer[result] = '\0'; // завершаем прочитанный текст нулём для printf
        printf("%s", buffer); // выводим прочитанную часть текста
        length -= (size_t)result; // уменьшаем количество оставшихся байт
    }
    return 0;
}

static int process_file(int fd) // обрабатываем запросы номеров строк
{
    struct Line *table = NULL; // динамическая таблица строк
    size_t count = 0; // количество строк в таблице
    char input[64]; // буфер для ввода номера строки
    char *end; // первый символ после разобранного числа
    char *result; // результат чтения номера строки
    long number; // номер строки, введённый пользователем
    int ch; // символ при чтении остатка длинного ввода
    int first_input = 1; // ограничиваем время только первой попытки ввода
    int status = 0; // пока ошибок нет

    if (make_table(fd, &table, &count) == -1) // сначала индексируем все строки файла
    {
        free(table); // освобождаем уже созданные записи при ошибке
        return 1;
    }

    while (1)
    {
        printf("Line number (0 to exit): ");
        fflush(stdout); // приглашение видно до ожидания ввода
        timed_out = 0; // сбрасываем флаг перед новым запросом
        if (first_input) // последующие запросы работают без ограничения времени
        {
            alarm(5); // пять секунд только на первый ввод
            first_input = 0;
        }
        result = fgets(input, sizeof(input), stdin); // читаем номер строки, первый ввод ограничен таймером
        if (result != NULL && strchr(input, '\n') == NULL && !feof(stdin) && !timed_out)
        {
            ch = fgetc(stdin); // ждём конец номера, даже если буфер заполнился
            if (ch != '\n' && ch != EOF)
            {
                input[0] = '\0'; // слишком длинный номер отклоняем целиком
                while (ch != '\n' && ch != EOF && !timed_out)
                {
                    ch = fgetc(stdin);
                }
            }
            if (ferror(stdin))
            {
                result = NULL;
            }
        }
        alarm(0); // отменяем таймер после ввода
        if (timed_out) // пять секунд истекли: выводим файл и завершаемся
        {
            printf("\n");
            if (count > 0 && print_text(fd, 0, (size_t)(table[count - 1].offset + table[count - 1].length)) == -1)
            {
                status = 1;
            }
            break;
        }
        if (result == NULL) // ввод завершён или произошла ошибка чтения
        {
            if (ferror(stdin))
            {
                perror("fgets");
                status = 1;
            }
            break;
        }

        errno = 0; // сбрасываем ошибку перед преобразованием числа
        number = strtol(input, &end, 10); // переводим номер из строки в десятичное число
        if (errno != 0 || end == input || (*end != '\n' && *end != '\0') ||
            number < 0 || (unsigned long)number > count) // отклоняем ошибки преобразования и номера вне таблицы
        {
            fprintf(stderr, "Invalid line number\n");
            continue;
        }
        if (number == 0) // ноль означает завершение работы
        {
            break;
        }
        if (print_text(fd, table[number - 1].offset, table[number - 1].length) == -1)
        {
            status = 1;
            break;
        }
    }

    free(table); // освобождаем таблицу строк перед выходом
    return status;
}

int main(int argc, char *argv[])
{
    int fd; // дескриптор открытого файла
    int status; // код завершения программы
    struct sigaction action; // настройки обработчика SIGALRM

    if (argc != 2) // ожидаем ровно один аргумент: путь к файлу
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    action.sa_handler = timeout_handler; // назначаем функцию для сигнала таймера
    sigemptyset(&action.sa_mask); // не блокируем дополнительные сигналы в обработчике
    action.sa_flags = 0; // fgets должен прерываться сигналом SIGALRM
    if (sigaction(SIGALRM, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    fd = open(argv[1], O_RDONLY); // открываем указанный файл только для чтения
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    status = process_file(fd); // строим таблицу и обрабатываем запросы
    if (close(fd) == -1) // закрываем файловый дескриптор
    {
        perror("close");
        status = 1;
    }
    return status;
}
