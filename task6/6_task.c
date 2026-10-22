#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 4096

typedef struct {
    off_t offset;
    size_t length;
} line_t;

static int fd;

static void timeout(int signal_number)
{
    char symbol;
    const char message[] = "\nTime is over. Complete file:\n";

    (void)signal_number;
    write(STDOUT_FILENO, message, sizeof message - 1);
    lseek(fd, 0L, SEEK_SET);
    while (read(fd, &symbol, 1) == 1)
        write(STDOUT_FILENO, &symbol, 1);
    close(fd);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    line_t table[MAX_LINES];
    char symbol, text[MAX_LINE_LENGTH], input[100], *end;
    off_t start = 0, position;
    ssize_t result;
    int lines = 0, number, first_input = 1;
    long value;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    signal(SIGALRM, timeout);

    while ((result = read(fd, &symbol, 1)) == 1) {
        if (symbol == '\n') {
            position = lseek(fd, 0L, SEEK_CUR);
            table[lines].offset = start;
            table[lines].length = (size_t)(position - start);
            start = position;
            if (++lines == MAX_LINES)
                break;
        }
    }
    if (result == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    position = lseek(fd, 0L, SEEK_CUR);
    if (position > start && lines < MAX_LINES) {
        table[lines].offset = start;
        table[lines].length = (size_t)(position - start);
        ++lines;
    }

    for (;;) {
        if (first_input)
            printf("Line number (5 seconds, 0 to quit): ");
        else
            printf("Line number (0 to quit): ");
        fflush(stdout);
        if (first_input)
            alarm(5);
        if (fgets(input, sizeof input, stdin) == NULL)
            break;
        if (first_input) {
            alarm(0);
            first_input = 0;
        }

        value = strtol(input, &end, 10);
        while (*end == ' ' || *end == '\t')
            ++end;
        if (end == input || (*end != '\n' && *end != '\0')) {
            puts("Invalid line number");
            continue;
        }
        if (value == 0)
            break;

        if (value < 1 || value > lines) {
            printf("File has %d lines\n", lines);
            continue;
        }
        number = (int)value;
        if (table[number - 1].length >= sizeof text) {
            puts("Line is too long");
            continue;
        }

        lseek(fd, table[number - 1].offset, SEEK_SET);
        result = read(fd, text, table[number - 1].length);
        if (result != (ssize_t)table[number - 1].length) {
            perror("read");
            break;
        }
        text[result] = '\0';
        printf("%s", text);
        if (result == 0 || text[result - 1] != '\n')
            putchar('\n');
    }

    alarm(0);
    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
