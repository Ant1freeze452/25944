#define _XOPEN_SOURCE 500

#include <stdio.h> // printf, fgets, perror
#include <stdlib.h> // realloc, free, strtol
#include <unistd.h> // read, lseek, close
#include <fcntl.h> // open, O_RDONLY
#include <errno.h> // errno, EINTR

struct Line
{
    off_t offset; // начало строки в файле
    size_t length; // длина вместе с переводом строки
};

static int make_table(int fd, struct Line **table, size_t *count)
{
    struct Line *next;
    off_t offset;
    size_t length;
    char ch;
    ssize_t result;

    while (1)
    {
        offset = lseek(fd, 0L, SEEK_CUR); // текущая позиция станет началом строки
        if (offset == (off_t)-1)
        {
            perror("lseek");
            return -1;
        }
        length = 0;
        while (1)
        {
            result = read(fd, &ch, 1); // ищем перевод строки
            if (result == -1 && errno == EINTR)
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
                break;
            }
            length++;
            if (ch == '\n')
            {
                break;
            }
        }

        if (length == 0)
        {
            break;
        }
        next = realloc(*table, (*count + 1) * sizeof(**table));
        if (next == NULL)
        {
            perror("realloc");
            return -1;
        }
        *table = next;
        (*table)[*count].offset = offset;
        (*table)[*count].length = length;
        (*count)++;
        if (result == 0) // последняя строка может не иметь перевода строки
        {
            break;
        }
    }
    return 0;
}

static int print_text(int fd, off_t offset, size_t length)
{
    char buffer[1025];
    size_t portion;
    ssize_t result;

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
    {
        perror("lseek");
        return -1;
    }
    while (length > 0)
    {
        portion = length < sizeof(buffer) - 1 ? length : sizeof(buffer) - 1;
        result = read(fd, buffer, portion); // читаем только длину выбранной строки
        if (result == -1 && errno == EINTR)
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
        buffer[result] = '\0';
        printf("%s", buffer);
        length -= (size_t)result;
    }
    return 0;
}

static int process_file(int fd)
{
    struct Line *table = NULL;
    size_t count = 0;
    char input[64];
    char *end;
    long number;
    int status = 0;

    if (make_table(fd, &table, &count) == -1)
    {
        free(table);
        return 1;
    }

    while (1)
    {
        printf("Line number (0 to exit): ");
        fflush(stdout); // приглашение видно до ожидания ввода
        if (fgets(input, sizeof(input), stdin) == NULL)
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
        if (print_text(fd, table[number - 1].offset, table[number - 1].length) == -1)
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

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    status = process_file(fd);
    if (close(fd) == -1)
    {
        perror("close");
        status = 1;
    }
    return status;
}
