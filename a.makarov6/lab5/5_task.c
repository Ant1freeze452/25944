#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 4096

typedef struct
{
    off_t offset;
    size_t length;
} line_t;

int main(int argc, char **argv)
{
    line_t table[MAX_LINES];
    char symbol, text[MAX_LINE_LENGTH];
    char input[100], *end;
    off_t start = 0, position;
    ssize_t result;
    int fd, lines = 0, number, i;
    long value;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return EXIT_FAILURE;
    }

    while ((result = read(fd, &symbol, 1)) == 1)
    {
        if (symbol == '\n')
        {
            position = lseek(fd, 0L, SEEK_CUR);

            table[lines].offset = start;
            table[lines].length = (size_t)(position - start);

            start = position;

            if (++lines == MAX_LINES)
                break;
        }
    }

    if (result == -1)
    {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    position = lseek(fd, 0L, SEEK_CUR);

    if (position > start && lines < MAX_LINES)
    {
        table[lines].offset = start;
        table[lines].length = (size_t)(position - start);
        ++lines;
    }

    printf("Offset table:\n");
    for (i = 0; i < lines; ++i)
    {
        printf("Line %d: offset = %ld, length = %lu\n",
            i + 1,
            (long)table[i].offset,
            (unsigned long)table[i].length);
    }

    for (;;)
    {
        printf("Line number (0 to quit): ");
        fflush(stdout);

        if (fgets(input, sizeof input, stdin) == NULL)
            break;

        value = strtol(input, &end, 10);

        while (*end == ' ' || *end == '\t')
            ++end;

        if (end == input || (*end != '\n' && *end != '\0'))
        {
            puts("Invalid line number");
            continue;
        }

        if (value == 0)
        {
            printf("Complete file:\n");
            fflush(stdout);

            if (lseek(fd, 0L, SEEK_SET) == -1)
            {
                perror("lseek");
                break;
            }

            while ((result = read(fd, &symbol, 1)) == 1)
                write(STDOUT_FILENO, &symbol, 1);

            break;
        }

        if (value < 1 || value > lines)
        {
            printf("File has %d lines\n", lines);
            continue;
        }
        number = (int)value;

        if (table[number - 1].length >= sizeof text)
        {
            puts("Line is too long");
            continue;
        }

        lseek(fd, table[number - 1].offset, SEEK_SET);

        result = read(fd, text, table[number - 1].length);

        if (result != (ssize_t)table[number - 1].length)
        {
            perror("read");
            break;
        }

        text[result] = '\0';

        printf("%s", text);

        if (result == 0 || text[result - 1] != '\n')
            putchar('\n');
    }

    if (close(fd) == -1)
    {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}