#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

volatile sig_atomic_t time_over = 0;

void alarm_handler(int signal_number) {
    (void)signal_number;
    time_over = 1;
}

int add_line(LineInfo **table, size_t *count,
             size_t *capacity, off_t start, off_t end) {
    if (*count == *capacity) {
        size_t new_size = *capacity ? *capacity * 2 : 16;

        LineInfo *temp = realloc(
            *table, new_size * sizeof(LineInfo)
        );

        if (temp == NULL)
            return 0;

        *table = temp;
        *capacity = new_size;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = (size_t)(end - start);
    (*count)++;

    return 1;
}

int build_table(int fd, LineInfo **table, size_t *count) {
    size_t capacity = 0;
    off_t start = 0;
    char ch;
    ssize_t n;

    while ((n = read(fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            off_t end = lseek(fd, 0, SEEK_CUR);

            if (end == (off_t)-1 ||
                !add_line(table, count, &capacity, start, end))
                return 0;

            start = end;
        }
    }

    if (n < 0)
        return 0;

    off_t end = lseek(fd, 0, SEEK_CUR);

    if (end == (off_t)-1)
        return 0;

    if (end > start) {
        if (!add_line(table, count, &capacity, start, end))
            return 0;
    }

    return 1;
}

int show_line(int fd, LineInfo info) {
    if (lseek(fd, info.offset, SEEK_SET) == (off_t)-1)
        return 0;

    char buffer[256];
    size_t remaining = info.length;
    char last = '\0';

    while (remaining > 0) {
        size_t amount = remaining < sizeof(buffer)
                      ? remaining : sizeof(buffer);

        ssize_t n = read(fd, buffer, amount);

        if (n <= 0)
            return 0;

        if (fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n)
            return 0;

        last = buffer[n - 1];
        remaining -= (size_t)n;
    }

    if (info.length == 0 || last != '\n')
        printf("\n");

    return 1;
}

int show_file(int fd) {
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
        return 0;

    char buffer[1024];
    ssize_t n;
    char last = '\0';

    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n)
            return 0;

        last = buffer[n - 1];
    }

    if (n < 0)
        return 0;

    if (last != '\n')
        printf("\n");

    return 1;
}

int read_number(long *number) {
    char input[64];

    if (fgets(input, sizeof(input), stdin) == NULL) {
        if (time_over)
            return -1;
        return 0;
    }

    char *end;
    errno = 0;
    long value = strtol(input, &end, 10);

    if (end == input || errno == ERANGE ||
        (*end != '\n' && *end != '\0'))
        return 2;

    *number = value;
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
    int status = 0;

    if (!build_table(fd, &table, &count)) {
        fprintf(stderr, "Ошибка построения таблицы\n");
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

    struct sigaction action = {0};
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(table);
        close(fd);
        return 1;
    }

    int first_request = 1;

    while (1) {
        long number = 0;

        printf("\nВведите номер строки (0 - выход): ");
        fflush(stdout);

        if (first_request) {
            time_over = 0;
            alarm(5);
        }

        int result = read_number(&number);

        if (first_request) {
            alarm(0);
            first_request = 0;
        }

        if (time_over || result == -1) {
            printf("\nВремя вышло! Содержимое файла:\n");

            if (!show_file(fd)) {
                perror("Чтение файла");
                status = 1;
            }

            break;
        }

        if (result == 0)
            break;

        if (result == 2) {
            printf("Введите корректный номер\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 0 || (unsigned long)number > count) {
            printf("Такой строки нет\n");
            continue;
        }

        if (!show_line(fd, table[number - 1])) {
            fprintf(stderr, "Ошибка чтения строки\n");
            status = 1;
            break;
        }
    }

    free(table);
    close(fd);

    return status;
}