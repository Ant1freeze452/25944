#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

int add_line(LineInfo **table, size_t *count, size_t *capacity,
             off_t start, off_t end) {
    if (*count == *capacity) {
        size_t new_capacity = *capacity ? *capacity * 2 : 16;
        LineInfo *tmp = realloc(*table, new_capacity * sizeof(LineInfo));

        if (tmp == NULL)
            return 0;

        *table = tmp;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = (size_t)(end - start);
    (*count)++;

    return 1;
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

    LineInfo *table = NULL;
    size_t count = 0;
    size_t capacity = 0;

    off_t start = 0;
    char ch;
    ssize_t bytes;

    while ((bytes = read(fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            off_t end = lseek(fd, 0L, SEEK_CUR);

            if (end == (off_t)-1) {
                perror("lseek");
                free(table);
                close(fd);
                return 1;
            }

            if (!add_line(&table, &count, &capacity, start, end)) {
                perror("realloc");
                free(table);
                close(fd);
                return 1;
            }

            start = end;
        }
    }

    if (bytes == -1) {
        perror("read");
        free(table);
        close(fd);
        return 1;
    }

    off_t end = lseek(fd, 0L, SEEK_END);

    if (end == (off_t)-1) {
        perror("lseek");
        free(table);
        close(fd);
        return 1;
    }

    /* Последняя строка добавляется всегда, даже пустая:
       если файл кончается на '\n', в таблице будет строка длины 0. */
    if (!add_line(&table, &count, &capacity, start, end)) {
        perror("realloc");
        free(table);
        close(fd);
        return 1;
    }

    printf("Таблица строк:\n");

    for (size_t i = 0; i < count; i++) {
        printf("%zu: offset = %lld, length = %zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }

    while (1) {
        char input[64];
        char *endptr;

        printf("Введите номер строки (0 - выход): ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        errno = 0;
        long number = strtol(input, &endptr, 10);

        if (endptr == input || errno == ERANGE ||
            (*endptr != '\n' && *endptr != '\0')) {
            printf("Введите корректный номер\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 0 || (unsigned long)number > count) {
            printf("Такой строки нет\n");
            continue;
        }

        LineInfo info = table[number - 1];

        if (lseek(fd, info.offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            break;
        }

        char *buffer = malloc(info.length + 1);

        if (buffer == NULL) {
            perror("malloc");
            break;
        }

        size_t total = 0;

        while (total < info.length) {
            ssize_t n = read(fd, buffer + total, info.length - total);

            if (n <= 0)
                break;

            total += (size_t)n;
        }

        if (total != info.length) {
            printf("Ошибка чтения строки\n");
            free(buffer);
            break;
        }

        buffer[total] = '\0';
        printf("%s", buffer);

        if (total == 0 || buffer[total - 1] != '\n')
            printf("\n");

        free(buffer);
    }

    free(table);
    close(fd);

    return 0;
}