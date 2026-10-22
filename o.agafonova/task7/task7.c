#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

typedef struct {
    size_t offset;
    size_t length;
} LineInfo;

volatile sig_atomic_t time_over = 0;

void alarm_handler(int sig) {
    (void)sig;
    time_over = 1;
}

int add_line(LineInfo **table, size_t *count,
             size_t *capacity, size_t start, size_t end) {
    if (*count == *capacity) {
        if (*capacity > SIZE_MAX / 2 / sizeof(LineInfo))
            return 0;

        size_t new_capacity = *capacity ? *capacity * 2 : 16;

        LineInfo *temp = realloc(
            *table, new_capacity * sizeof(LineInfo)
        );

        if (temp == NULL)
            return 0;

        *table = temp;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = end - start;
    (*count)++;

    return 1;
}

int build_table(const char *data, size_t size,
                LineInfo **table, size_t *count) {
    size_t capacity = 0;
    size_t start = 0;

    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            if (!add_line(table, count, &capacity, start, i + 1))
                return 0;

            start = i + 1;
        }
    }

    if (start < size) {
        if (!add_line(table, count, &capacity, start, size))
            return 0;
    }

    return 1;
}

void print_line(const char *data, LineInfo line) {
    fwrite(data + line.offset, 1, line.length, stdout);

    if (line.length == 0 ||
        data[line.offset + line.length - 1] != '\n')
        putchar('\n');
}

void print_file(const char *data, size_t size) {
    if (size > 0)
        fwrite(data, 1, size, stdout);

    if (size > 0 && data[size - 1] != '\n')
        putchar('\n');
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Использование: %s <файл>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat file_info;

    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (file_info.st_size < 0 ||
        (uintmax_t)file_info.st_size > SIZE_MAX) {
        fprintf(stderr, "Недопустимый размер файла\n");
        close(fd);
        return 1;
    }

    size_t file_size = (size_t)file_info.st_size;
    char *data = NULL;

    if (file_size > 0) {
        data = mmap(NULL, file_size, PROT_READ,
                    MAP_PRIVATE, fd, 0);

        if (data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    LineInfo *table = NULL;
    size_t count = 0;

    if (!build_table(data, file_size, &table, &count)) {
        fprintf(stderr, "Ошибка создания таблицы\n");

        free(table);

        if (file_size > 0)
            munmap(data, file_size);

        close(fd);
        return 1;
    }

    printf("Таблица строк:\n");

    for (size_t i = 0; i < count; i++) {
        printf("%zu: offset = %zu, length = %zu\n",
               i + 1, table[i].offset, table[i].length);
    }

    struct sigaction action = {0};
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(table);

        if (file_size > 0)
            munmap(data, file_size);

        close(fd);
        return 1;
    }

    char input[64];
    int first_request = 1;

    while (1) {
        printf("\nВведите номер строки (0 - выход): ");
        fflush(stdout);

        if (first_request) {
            time_over = 0;
            alarm(5);
        }

        char *result = fgets(input, sizeof(input), stdin);

        if (first_request) {
            alarm(0);
            first_request = 0;
        }

        if (time_over) {
            printf("\nВремя вышло! Содержимое файла:\n");
            print_file(data, file_size);
            break;
        }

        if (result == NULL)
            break;

        char *end;
        errno = 0;
        long number = strtol(input, &end, 10);

        if (end == input || errno == ERANGE ||
            (*end != '\n' && *end != '\0')) {
            printf("Некорректный номер строки\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 0 || (unsigned long)number > count) {
            printf("Такой строки нет\n");
            continue;
        }

        print_line(data, table[number - 1]);
    }

    free(table);

    if (file_size > 0)
        munmap(data, file_size);

    close(fd);
    return 0;
}