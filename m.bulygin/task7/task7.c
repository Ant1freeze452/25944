#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct Line
{
    off_t offset;
    size_t length;
};

volatile sig_atomic_t timeout = 0;

void alarm_handler(int sig)
{
    timeout = 1;
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat st;
    char *data;

    struct Line *table = NULL;
    size_t count = 0;
    size_t capacity = 10;

    size_t i;
    off_t line_start = 0;

    char input[100];
    char *end;
    long line_number;

    int first_input = 1;

    struct sigaction sa;

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

    if (fstat(fd, &st) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0)
    {
        printf("File is empty\n");
        close(fd);
        return 0;
    }

    data = mmap(NULL,
                st.st_size,
                PROT_READ,
                MAP_PRIVATE,
                fd,
                0);

    if (data == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    table = malloc(capacity * sizeof(struct Line));

    if (table == NULL)
    {
        perror("malloc");
        munmap(data, st.st_size);
        close(fd);
        return 1;
    }

    for (i = 0; i < (size_t)st.st_size; i++)
    {
        if (data[i] == '\n')
        {
            if (count == capacity)
            {
                struct Line *temp;

                capacity *= 2;

                temp = realloc(table,
                               capacity * sizeof(struct Line));

                if (temp == NULL)
                {
                    perror("realloc");
                    free(table);
                    munmap(data, st.st_size);
                    close(fd);
                    return 1;
                }

                table = temp;
            }

            table[count].offset = line_start;
            table[count].length =
                i - line_start + 1;

            count++;
            line_start = i + 1;
        }
    }

    if (line_start < st.st_size)
    {
        if (count == capacity)
        {
            struct Line *temp;

            capacity *= 2;

            temp = realloc(table,
                           capacity * sizeof(struct Line));

            if (temp == NULL)
            {
                perror("realloc");
                free(table);
                munmap(data, st.st_size);
                close(fd);
                return 1;
            }

            table = temp;
        }

        table[count].offset = line_start;
        table[count].length =
            st.st_size - line_start;

        count++;
    }

    printf("Line table:\n");

    for (i = 0; i < count; i++)
    {
        printf("%lu: offset = %ld, length = %lu\n",
               (unsigned long)(i + 1),
               (long)table[i].offset,
               (unsigned long)table[i].length);
    }

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("sigaction");
        free(table);
        munmap(data, st.st_size);
        close(fd);
        return 1;
    }

    while (1)
    {
        if (first_input)
        {
            timeout = 0;

            printf("\nEnter line number (0 to exit, 5 seconds): ");
            fflush(stdout);

            alarm(5);
        }
        else
        {
            printf("\nEnter line number (0 to exit): ");
            fflush(stdout);
        }

        errno = 0;

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if (first_input && timeout)
            {
                alarm(0);

                printf("\nTime is over. File contents:\n\n");

                fwrite(data,
                       1,
                       st.st_size,
                       stdout);

                if (data[st.st_size - 1] != '\n')
                {
                    printf("\n");
                }

                break;
            }

            break;
        }

        if (first_input)
        {
            alarm(0);
            first_input = 0;
        }

        line_number = strtol(input, &end, 10);

        if (end == input)
        {
            printf("Invalid input\n");
            continue;
        }

        while (*end == ' ' || *end == '\t')
        {
            end++;
        }

        if (*end != '\n' && *end != '\0')
        {
            printf("Invalid input\n");
            continue;
        }

        if (line_number == 0)
        {
            break;
        }

        if (line_number < 1 ||
            (size_t)line_number > count)
        {
            printf("No such line\n");
            continue;
        }

        fwrite(data +
                   table[line_number - 1].offset,
               1,
               table[line_number - 1].length,
               stdout);

        if (table[line_number - 1].length > 0 &&
            data[table[line_number - 1].offset +
                 table[line_number - 1].length - 1]
                != '\n')
        {
            printf("\n");
        }
    }

    free(table);
    munmap(data, st.st_size);
    close(fd);

    return 0;
}
