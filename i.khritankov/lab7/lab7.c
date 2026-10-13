#define _XOPEN_SOURCE 500

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

static void timeout_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1; // ввод/вывод внутри обработчика не выполняем
}

static int make_table(const char *data, size_t size, struct Line **table, size_t *count)
{
    struct Line *next;
    size_t start = 0;
    size_t end;

    while (start < size)
    {
        end = start;
        while (end < size && data[end] != '\n')
        {
            end++;
        }
        if (end < size)
        {
            end++; // включаем перевод строки в её длину
        }

        next = realloc(*table, (*count + 1) * sizeof(**table));
        if (next == NULL)
        {
            perror("realloc");
            return -1;
        }
        *table = next;
        (*table)[*count].offset = (off_t)start;
        (*table)[*count].length = end - start;
        (*count)++;
        start = end;
    }
    return 0;
}

static int print_text(const char *data, off_t offset, size_t length)
{
    size_t portion;

    while (length > 0)
    {
        portion = length < 1024 ? length : 1024;
        printf("%.*s", (int)portion, data + offset); // выводим прямо из отображения
        offset += (off_t)portion;
        length -= portion;
    }
    return 0;
}

static int process_file(const char *data, size_t size)
{
    struct Line *table = NULL;
    size_t count = 0;
    char input[64];
    char *end;
    char *result;
    long number;
    int ch;
    int status = 0;

    if (make_table(data, size, &table, &count) == -1)
    {
        free(table);
        return 1;
    }

    while (1)
    {
        printf("Line number (0 to exit): ");
        fflush(stdout); // приглашение видно до ожидания ввода
        timed_out = 0;
        alarm(5); // пять секунд на каждый запрос
        result = fgets(input, sizeof(input), stdin);
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
        if (timed_out)
        {
            printf("\n");
            if (count > 0 && print_text(data, 0, (size_t)(table[count - 1].offset + table[count - 1].length)) == -1)
            {
                status = 1;
            }
            break;
        }
        if (result == NULL)
        {
            if (ferror(stdin))
            {
                perror("fgets");
                status = 1;
            }
            break;
        }

        errno = 0;
        number = strtol(input, &end, 10);
        if (errno != 0 || end == input || (*end != '\n' && *end != '\0') ||
            number < 0 || (unsigned long)number > count)
        {
            fprintf(stderr, "Invalid line number\n");
            continue;
        }
        if (number == 0)
        {
            break;
        }
        if (print_text(data, table[number - 1].offset, table[number - 1].length) == -1)
        {
            status = 1;
            break;
        }
    }

    free(table);
    return status;
}

int main(int argc, char *argv[])
{
    int fd;
    int status;
    struct stat info;
    struct sigaction action;
    char *data = NULL;
    size_t size;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    action.sa_handler = timeout_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
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

    size = (size_t)info.st_size;
    if (size > 0) // пустой файл нельзя отображать через mmap
    {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    status = process_file(data, size);
    if (size > 0 && munmap(data, size) == -1)
    {
        perror("munmap");
        status = 1;
    }
    if (close(fd) == -1)
    {
        perror("close");
        status = 1;
    }
    return status;
}
