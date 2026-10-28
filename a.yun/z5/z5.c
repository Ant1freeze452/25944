#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct line {
    off_t offset;
    size_t length;
};

static int add_line(struct line **table, size_t *count,
                    off_t offset, size_t length)
{
    struct line *grown = realloc(*table, (*count + 1) * sizeof(**table));
    if (grown == NULL) {
        perror("realloc");
        return -1;
    }
    *table = grown;
    grown[*count].offset = offset;
    grown[*count].length = length;
    ++*count;
    return 0;
}

static int build_table(int fd, struct line **table, size_t *count)
{
    char byte;
    off_t start = 0, position;
    ssize_t received;
    while ((received = read(fd, &byte, 1)) > 0) {
        if (byte == '\n') {
            position = lseek(fd, 0, SEEK_CUR);
            if (position == -1) {
                perror("lseek");
                return -1;
            }
            if (add_line(table, count, start, (size_t)(position - start)) == -1)
                return -1;
            start = position;
        }
    }
    if (received == -1) {
        perror("read");
        return -1;
    }
    position = lseek(fd, 0, SEEK_CUR);
    if (position == -1) {
        perror("lseek");
        return -1;
    }
    if (position > start)
        return add_line(table, count, start, (size_t)(position - start));
    return 0;
}

static int print_line(int fd, struct line line)
{
    char *text = malloc(line.length + 1);
    size_t total = 0;
    if (text == NULL) {
        perror("malloc");
        return -1;
    }
    if (lseek(fd, line.offset, SEEK_SET) == -1) {
        perror("lseek");
        free(text);
        return -1;
    }
    while (total < line.length) {
        ssize_t received = read(fd, text + total, line.length - total);
        if (received <= 0) {
            fprintf(stderr, "Cannot read the requested line\n");
            free(text);
            return -1;
        }
        total += (size_t)received;
    }
    text[line.length] = '\0';
    printf("%s", text);
    free(text);
    return 0;
}

/* 1: positive number, 0: exactly "0", 2: invalid input, -1: EOF/error. */
static int read_number(long *number)
{
    char input[64], byte, *end;
    size_t used = 0;
    int invalid = 0;
    ssize_t received;

    while ((received = read(STDIN_FILENO, &byte, 1)) > 0) {
        if (byte == '\n')
            break;
        if (byte == '\0')
            invalid = 1;
        if (used < sizeof(input) - 1)
            input[used++] = byte;
        else
            invalid = 1;
    }
    if (received == -1) {
        perror("read");
        return -1;
    }
    if (received == 0 && used == 0 && !invalid) {
        fprintf(stderr, "Input closed before 0.\n");
        return -1;
    }
    input[used] = '\0';
    if (invalid)
        return 2;
    if (strcmp(input, "0") == 0)
        return 0;
    if (input[0] < '0' || input[0] > '9')
        return 2;
    errno = 0;
    *number = strtol(input, &end, 10);
    if (errno != 0 || *end != '\0' || *number <= 0)
        return 2;
    return 1;
}

int main(int argc, char **argv)
{
    int fd, status = 0;
    struct line *table = NULL;
    size_t count = 0;
    long number;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }
    if (build_table(fd, &table, &count) == -1) {
        status = 1;
        goto cleanup;
    }
    for (;;) {
        printf("Line number (0 to quit): ");
        fflush(stdout);
        int result = read_number(&number);
        if (result == -1) {
            status = 1;
            break;
        }
        if (result == 0)
            break;
        if (result == 2) {
            puts("Enter a positive line number or exactly 0 to quit.");
            continue;
        }
        if (number < 1 || (size_t)number > count) {
            puts("No such line.");
            continue;
        }
        if (print_line(fd, table[number - 1]) == -1) {
            status = 1;
            break;
        }
    }

cleanup:
    free(table);
    close(fd);
    return status;
}
