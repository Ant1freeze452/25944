#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 512

static off_t *offsets = NULL;   // отступ начала каждой строки
static long *lengths = NULL;    // длина каждой строки без '\n'
static long count = 0, capacity = 0;

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

int main(int argc, char *argv[])
{
    int fd;
    char buf[BUFSIZE];
    ssize_t n;
    off_t line_start = 0;       // первая строка начинается с нулевого отступа
    off_t pos;
    long number;

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

    // запрос номеров строк
    for (;;) {
        char input[256];
        char *end;
        char *line;

        printf("Номер строки (0 - выход): ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL)
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