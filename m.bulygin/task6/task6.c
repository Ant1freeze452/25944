#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

struct Line
{
    off_t offset;
    off_t length;
};

volatile sig_atomic_t timeout = 0;

void alarm_handler(int sig)
{
    timeout = 1;
}

void print_file(int fd)
{
    char buffer[1024];
    ssize_t n;

    if (lseek(fd, 0L, SEEK_SET) == -1)
    {
        perror("lseek");
        return;
    }

    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(STDOUT_FILENO, buffer, n) == -1)
        {
            perror("write");
            return;
        }
    }

    if (n == -1)
    {
        perror("read");
    }
}

int read_line_number(long *number)
{
    char buffer[100];
    int pos = 0;
    char c;
    ssize_t result;
    char *end;

    timeout = 0;
    alarm(5);

    while (pos < (int)sizeof(buffer) - 1)
    {
        result = read(STDIN_FILENO, &c, 1);

        if (result == -1)
        {
            if (errno == EINTR && timeout)
            {
                alarm(0);
                return 0;
            }

            perror("read");
            alarm(0);
            return -1;
        }

        if (result == 0)
        {
            alarm(0);
            return -1;
        }

        if (c == '\n')
        {
            break;
        }

        buffer[pos] = c;
        pos++;
    }

    alarm(0);

    if (timeout)
    {
        return 0;
    }

    buffer[pos] = '\0';

    *number = strtol(buffer, &end, 10);

    if (end == buffer || *end != '\0')
    {
        return 2;
    }

    return 1;
}

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
    size_t i;

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

    table = malloc(capacity * sizeof(struct Line));

    if (table == NULL)
    {
        perror("malloc");
        close(fd);
        return 1;
    }

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("sigaction");
        free(table);
        close(fd);
        return 1;
    }

    while ((result = read(fd, &c, 1)) > 0)
    {
        if (c == '\n')
        {
            position = lseek(fd, 0L, SEEK_CUR);

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

    position = lseek(fd, 0L, SEEK_CUR);

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

    for (i = 0; i < count; i++)
    {
        printf("%lu: offset = %ld, length = %ld\n",
               (unsigned long)(i + 1),
               (long)table[i].offset,
               (long)table[i].length);
    }

    while (1)
    {
        int input_result;

        printf("\nEnter line number (0 to exit, 5 seconds): ");
        fflush(stdout);

        input_result = read_line_number(&line_number);

        if (input_result == 0)
        {
            printf("\nTime is over. File contents:\n\n");
            print_file(fd);
            break;
        }

        if (input_result == -1)
        {
            break;
        }

        if (input_result == 2)
        {
            printf("Invalid input\n");
            continue;
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

        if (lseek(fd, table[line_number - 1].offset, SEEK_SET) == -1)
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
