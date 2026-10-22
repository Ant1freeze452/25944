#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000

typedef struct {
    size_t offset;
    size_t length;
} line_t;

static char *file_data;
static size_t file_size;

static void timeout(int signal_number)
{
    (void)signal_number;
    puts("\nTime is over. Complete file:");
    if (file_size != 0)
        fwrite(file_data, 1, file_size, stdout);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    line_t table[MAX_LINES];
    struct stat info;
    char input[100], *end;
    size_t start = 0, i;
    int fd, lines = 0, number, first_input = 1;
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
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    file_size = (size_t)info.st_size;
    if (file_size != 0) {
        file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (file_data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    close(fd);

    for (i = 0; i < file_size && lines < MAX_LINES; ++i) {
        if (file_data[i] == '\n') {
            table[lines].offset = start;
            table[lines].length = i - start + 1;
            start = i + 1;
            ++lines;
        }
    }
    if (start < file_size && lines < MAX_LINES) {
        table[lines].offset = start;
        table[lines].length = file_size - start;
        ++lines;
    }

    signal(SIGALRM, timeout);

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

        fwrite(file_data + table[number - 1].offset,
            1, table[number - 1].length, stdout);
        if (file_data[table[number - 1].offset +
            table[number - 1].length - 1] != '\n')
            putchar('\n');
    }

    alarm(0);
    if (file_size != 0)
        munmap(file_data, file_size);
    return EXIT_SUCCESS;
}
