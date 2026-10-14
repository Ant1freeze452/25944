#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

struct Line
{
    off_t offset;
    off_t length;
};

int main(int argc, char *argv[])
{
    int fd;
    char c;
    ssize_t result;
    off_t line_start = 0;
    off_t position;

    struct Line *table = NULL;
    size_t count = 0;
    size_t capacity = 10;

    long line_number;
    char *buffer;
    size_t total;
    size_t length;

    if (argc != 2)
    {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    table = malloc(capacity * sizeof(struct Line));

    if (table == NULL)
    {
        perror("malloc");
        close(fd);
        return 1;
    }

    while ((result = read(fd, &c, 1)) > 0)
    {
        if (c == '\n')
        {
            position = lseek(fd, 0L, 1);

            if (position == -1)
            {
                perror("lseek");
                free(table);
                close(fd);
                return 1;
            }

            if (count == capacity)
            {
                struct Line *temp;

                capacity *= 2;

                temp = realloc(table, capacity * sizeof(struct Line));

                if (temp == NULL)
                {
                    perror("realloc");
                    free(table);
                    close(fd);
                    return 1;
                }

                table = temp;
            }

            table[count].offset = line_start;
            table[count].length = position - line_start;

            count++;

            line_start = position;
        }
    }

    if (result == -1)
    {
        perror("read");
        free(table);
        close(fd);
        return 1;
    }

    position = lseek(fd, 0L, 1);

    if (position == -1)
    {
        perror("lseek");
        free(table);
        close(fd);
        return 1;
    }

    if (position > line_start)
    {
        if (count == capacity)
        {
            struct Line *temp;

            capacity *= 2;

            temp = realloc(table, capacity * sizeof(struct Line));

            if (temp == NULL)
            {
                perror("realloc");
                free(table);
                close(fd);
                return 1;
            }

            table = temp;
        }

        table[count].offset = line_start;
        table[count].length = position - line_start;

        count++;
    }

    printf("Line table:\n");

    {
        size_t i;

        for (i = 0; i < count; i++)
        {
            printf("%lu: offset = %ld, length = %ld\n",
                   (unsigned long)(i + 1),
                   (long)table[i].offset,
                   (long)table[i].length);
        }
    }

    while (1)
    {
        printf("\nEnter line number (0 to exit): ");

        if (scanf("%ld", &line_number) != 1)
        {
            printf("Invalid input\n");
            break;
        }

        if (line_number == 0)
        {
            break;
        }

        if (line_number < 1 || (size_t)line_number > count)
        {
            printf("No such line\n");
            continue;
        }

        if (lseek(fd, table[line_number - 1].offset, 0) == -1)
        {
            perror("lseek");
            break;
        }

        length = (size_t)table[line_number - 1].length;

        buffer = malloc(length + 1);

        if (buffer == NULL)
        {
            perror("malloc");
            break;
        }

        total = 0;

        while (total < length)
        {
            result = read(fd, buffer + total, length - total);

            if (result == -1)
            {
                perror("read");
                free(buffer);
                free(table);
                close(fd);
                return 1;
            }

            if (result == 0)
            {
                break;
            }

            total += (size_t)result;
        }

        buffer[total] = '\0';

        printf("%s", buffer);

        if (total > 0 && buffer[total - 1] != '\n')
        {
            printf("\n");
        }

        free(buffer);
    }

    free(table);
    close(fd);

    return 0;
}
