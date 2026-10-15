#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <stdio.h> // printf, fgets, perror
#include <stdlib.h> // realloc, free, strtol
#include <string.h> // strchr
#include <unistd.h> // alarm, close
#include <fcntl.h> // open, O_RDONLY
#include <errno.h> // errno
#include <sys/mman.h> // mmap, munmap
#include <sys/stat.h> // fstat
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

static int make_table(const char *data, size_t size, struct Line **table, size_t *count) // индексируем строки в отображении файла
{
    struct Line *next; // новый адрес таблицы после realloc
    size_t start = 0; // начало текущей строки в отображении
    size_t end; // позиция сразу после текущей строки

    while (start < size)
    {
        end = start; // начинаем поиск конца строки
        while (end < size && data[end] != '\n') // ищем перевод строки, не выходя за размер файла
        {
            end++;
        }
        if (end < size)
        {
            end++; // включаем перевод строки в её длину
        }

        next = realloc(*table, (*count + 1) * sizeof(**table)); // расширяем таблицу на одну запись
        if (next == NULL)
        {
            perror("realloc");
            return -1;
        }
        *table = next; // сохраняем новый адрес выделенной памяти
        (*table)[*count].offset = (off_t)start; // сохраняем смещение начала строки
        (*table)[*count].length = end - start; // вычисляем длину строки по двум позициям
        (*count)++; // увеличиваем количество записей
        start = end; // переходим к следующей строке
    }
    return 0;
}

static int print_text(const char *data, off_t offset, size_t length) // выводим диапазон прямо из отображения
{
    size_t portion; // размер очередной порции вывода

    while (length > 0)
    {
        portion = length < 1024 ? length : 1024; // ограничиваем одну порцию вывода 1024 байтами
        printf("%.*s", (int)portion, data + offset); // выводим прямо из отображения
        offset += (off_t)portion; // сдвигаем начало следующей порции
        length -= portion; // учитываем уже выведенные байты
    }
    return 0;
}

static int process_file(const char *data, size_t size) // обрабатываем запросы к отображённому файлу
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

    if (make_table(data, size, &table, &count) == -1) // строим таблицу по отображённым данным
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
            if (count > 0 && print_text(data, 0, (size_t)(table[count - 1].offset + table[count - 1].length)) == -1)
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
        if (print_text(data, table[number - 1].offset, table[number - 1].length) == -1)
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
    struct stat info; // размер и другие атрибуты файла
    struct sigaction action; // настройки обработчика SIGALRM
    char *data = NULL; // адрес отображения, у пустого файла его нет
    size_t size; // размер файла в байтах

    if (argc != 2) // ожидаем ровно один аргумент: путь к файлу
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    action.sa_handler = timeout_handler; // назначаем функцию для сигнала таймера
    sigemptyset(&action.sa_mask); // не блокируем дополнительные сигналы в обработчике
    action.sa_flags = 0;
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
    if (fstat(fd, &info) == -1) // узнаём размер для mmap
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    size = (size_t)info.st_size; // сохраняем размер, полученный через fstat
    if (size > 0) // пустой файл нельзя отображать через mmap
    {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0); // отображаем весь файл только для чтения
        if (data == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    status = process_file(data, size); // ищем строки непосредственно в отображённой памяти
    if (size > 0 && munmap(data, size) == -1) // удаляем отображение файла перед выходом
    {
        perror("munmap");
        status = 1;
    }
    if (close(fd) == -1) // закрываем файловый дескриптор
    {
        perror("close");
        status = 1;
    }
    return status;
}
