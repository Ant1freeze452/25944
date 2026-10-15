#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 512
#define TIMEOUT 5

static off_t *offsets = NULL;   // отступ начала каждой строки
static long *lengths = NULL;    // длина каждой строки без '\n'
static long count = 0, capacity = 0;

static volatile sig_atomic_t timed_out = 0;

// обработчик SIGALRM: только ставим флаг
static void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

// добавление строки в таблицу
static void add_line(off_t offset, long length)
{
    if (count == capacity) {
        capacity = capacity ? capacity * 2 : 16;
        offsets = realloc(offsets, capacity * sizeof(*offsets));
        lengths = realloc(lengths, capacity * sizeof(*lengths));
        if (offsets == NULL || lengths == NULL) {
            perror("realloc");
            exit(1);
        }
    }
    offsets[count] = offset;
    lengths[count] = length;
    count++;
}

// печать всего содержимого файла
static void print_file(int fd)
{
    char buf[BUFSIZE];
    ssize_t n;

    fflush(stdout);
    lseek(fd, 0L, SEEK_SET);
    while ((n = read(fd, buf, BUFSIZE)) > 0)
        write(STDOUT_FILENO, buf, n);
    if (n == -1)
        perror("read");
}

int main(int argc, char *argv[])
{
    int fd;
    char buf[BUFSIZE];
    ssize_t n;
    off_t line_start = 0;       // первая строка начинается с нулевого отступа
    off_t pos;
    long number;
    struct sigaction sa;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s файл\n", argv[0]);
        return 1;
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror(argv[1]);
        return 1;
    }

    // строим таблицу отступов и длин строк
    while ((n = read(fd, buf, BUFSIZE)) > 0) {
        // позиция в файле, с которой начинается прочитанный буфер
        pos = lseek(fd, 0L, SEEK_CUR) - n;

        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                add_line(line_start, pos + i - line_start);
                line_start = pos + i + 1;
            }
        }
    }
    if (n == -1) {
        perror("read");
        return 1;
    }

    // последняя строка без '\n' в конце файла
    pos = lseek(fd, 0L, SEEK_CUR);
    if (pos > line_start)
        add_line(line_start, pos - line_start);

    // отладочная печать таблицы
    printf("строка  отступ  длина\n");
    for (long i = 0; i < count; i++)
        printf("%6ld  %6lld  %5ld\n", i + 1, (long long)offsets[i], lengths[i]);

    // без SA_RESTART, чтобы SIGALRM прерывал ожидание ввода
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // запрос номеров строк, таймер только на первый ввод
    for (int first = 1;; first = 0) {
        char input[256];
        char *end;
        char *line;
        char *got;

        if (first)
            printf("Номер строки (0 - выход, %d сек): ", TIMEOUT);
        else
            printf("Номер строки (0 - выход): ");
        fflush(stdout);

        if (first)
            alarm(TIMEOUT);
        got = fgets(input, sizeof(input), stdin);
        alarm(0);

        if (timed_out) {
            printf("\nВремя вышло, содержимое файла:\n");
            print_file(fd);
            break;
        }
        if (got == NULL)
            break;

        // число, вокруг которого допускаются только пробелы
        number = strtol(input, &end, 10);
        if (end == input) {
            printf("Нужно ввести число\n");
            continue;
        }
        while (isspace((unsigned char)*end))
            end++;
        if (*end != '\0') {
            printf("Нужно ввести число\n");
            continue;
        }
        if (number == 0)
            break;
        if (number < 0 || number > count) {
            printf("Нет строки с номером %ld (в файле %ld строк)\n",
                   number, count);
            continue;
        }

        line = malloc(lengths[number - 1] + 1);
        if (line == NULL) {
            perror("malloc");
            return 1;
        }
        // позиционируемся на начало строки и читаем ровно её длину
        lseek(fd, offsets[number - 1], SEEK_SET);
        n = read(fd, line, lengths[number - 1]);
        if (n == -1) {
            perror("read");
            free(line);
            return 1;
        }
        line[n] = '\0';
        printf("%s\n", line);
        free(line);
    }

    free(offsets);
    free(lengths);
    close(fd);
    return 0;
}