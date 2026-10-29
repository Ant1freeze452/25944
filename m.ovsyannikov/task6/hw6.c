#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define CHUNK       512
#define TIMEOUT_SEC 5

typedef struct {
    off_t  offset;   /* смещение начала строки в файле */
    size_t length;   /* длина строки без '\n' */
} LineInfo;

typedef struct {
    LineInfo *items;
    size_t    count;
    size_t    capacity;
} LineTable;

static int table_push(LineTable *t, off_t offset, size_t length)
{
    if (t->count == t->capacity) {
        size_t new_cap = t->capacity ? t->capacity * 2 : 16;
        LineInfo *p = realloc(t->items, new_cap * sizeof(*p));
        if (p == NULL)
            return -1;
        t->items = p;
        t->capacity = new_cap;
    }
    t->items[t->count].offset = offset;
    t->items[t->count].length = length;
    t->count++;
    return 0;
}

/* Проходит файл блоками, находит каждый '\n' и заполняет таблицу */
static int build_table(int fd, LineTable *t)
{
    char    chunk[CHUNK];
    ssize_t got;
    off_t   line_start = 0;          /* первая строка начинается с 0 */

    if (lseek(fd, 0L, SEEK_SET) == (off_t)-1)
        return -1;

    while ((got = read(fd, chunk, sizeof(chunk))) > 0) {
        /* текущая позиция в файле — сразу за прочитанным блоком */
        off_t pos_after = lseek(fd, 0L, SEEK_CUR);
        if (pos_after == (off_t)-1)
            return -1;
        off_t chunk_start = pos_after - got;

        for (ssize_t i = 0; i < got; i++) {
            if (chunk[i] == '\n') {
                off_t nl_pos = chunk_start + i;
                if (table_push(t, line_start, (size_t)(nl_pos - line_start)) < 0)
                    return -1;
                line_start = nl_pos + 1;
            }
        }
    }
    if (got < 0)
        return -1;

    /* последняя строка без завершающего '\n' */
    off_t end = lseek(fd, 0L, SEEK_CUR);
    if (end == (off_t)-1)
        return -1;
    if (end > line_start &&
        table_push(t, line_start, (size_t)(end - line_start)) < 0)
        return -1;

    return 0;
}

static void dump_table(const LineTable *t)
{
    printf("  N   offset  length\n");
    for (size_t i = 0; i < t->count; i++)
        printf("%3zu %8lld %7zu\n", i + 1,
               (long long)t->items[i].offset, t->items[i].length);
}

/* Позиционируется на начало строки и читает ровно её длину */
static int print_line(int fd, const LineInfo *li)
{
    char *text = malloc(li->length + 1);
    if (text == NULL)
        return -1;

    if (lseek(fd, li->offset, SEEK_SET) == (off_t)-1) {
        free(text);
        return -1;
    }

    size_t done = 0;
    while (done < li->length) {
        ssize_t r = read(fd, text + done, li->length - done);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            free(text);
            return -1;
        }
        if (r == 0)
            break;
        done += (size_t)r;
    }
    text[done] = '\0';

    printf("%s\n", text);
    free(text);
    return 0;
}

/* ---------- таймаут на ввод ---------- */

static volatile sig_atomic_t timed_out = 0;

static void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

/* Читает строку со стандартного ввода через read(2), по одному байту,
   чтобы не захватить лишнее из следующих строк.
   Возвращает длину, 0 при конце ввода, -1 при ошибке, -2 при таймауте. */
static ssize_t read_line(char *buf, size_t size)
{
    size_t n = 0;

    while (n + 1 < size) {
        char c;
        ssize_t r = read(STDIN_FILENO, &c, 1);

        if (r < 0) {
            if (errno == EINTR) {
                if (timed_out)
                    return -2;
                continue;
            }
            return -1;
        }
        if (r == 0)
            break;

        buf[n++] = c;
        if (c == '\n')
            break;
    }

    /* слишком длинная строка — дочитываем остаток до '\n' */
    if (n + 1 == size && buf[n - 1] != '\n') {
        char c;
        ssize_t r;
        while ((r = read(STDIN_FILENO, &c, 1)) == 1 && c != '\n')
            ;
        if (r < 0 && errno == EINTR && timed_out)
            return -2;
    }

    buf[n] = '\0';
    return (ssize_t)n;
}

/* Печатает весь файл с начала */
static int print_whole_file(int fd)
{
    char chunk[CHUNK];
    ssize_t got;
    char last = '\n';

    if (lseek(fd, 0L, SEEK_SET) == (off_t)-1)
        return -1;

    while ((got = read(fd, chunk, sizeof(chunk))) > 0) {
        last = chunk[got - 1];
        ssize_t done = 0;
        while (done < got) {
            ssize_t w = write(STDOUT_FILENO, chunk + done, (size_t)(got - done));
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            done += w;
        }
    }
    if (got < 0)
        return -1;
    if (last != '\n' && write(STDOUT_FILENO, "\n", 1) < 0)
        return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    int debug = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = 1;
        else
            path = argv[i];
    }
    if (path == NULL) {
        fprintf(stderr, "Usage: %s [-d] file\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror(path);
        return EXIT_FAILURE;
    }

    LineTable table = { NULL, 0, 0 };
    if (build_table(fd, &table) < 0) {
        perror("build_table");
        free(table.items);
        close(fd);
        return EXIT_FAILURE;
    }

    if (debug)
        dump_table(&table);

    if (table.count == 0) {
        printf("Файл %s пуст\n", path);
        free(table.items);
        close(fd);
        return EXIT_SUCCESS;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                 /* без SA_RESTART: read() прервётся */
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        free(table.items);
        close(fd);
        return EXIT_FAILURE;
    }

    char input[64];
    int first = 1;                   /* таймер только на первый ввод */

    for (;;) {
        if (first) {
            printf("Номер строки (1-%zu, 0 - выход), у вас %d секунд: ",
                   table.count, TIMEOUT_SEC);
            fflush(stdout);
            timed_out = 0;
            alarm(TIMEOUT_SEC);
        } else {
            printf("Номер строки (1-%zu, 0 - выход): ", table.count);
            fflush(stdout);
        }

        ssize_t got = read_line(input, sizeof(input));

        if (first) {
            alarm(0);                /* успели — дальше без ограничения */
            first = 0;
        }

        if (got == -2) {             /* время вышло */
            printf("\nВремя вышло. Содержимое файла:\n");
            fflush(stdout);
            if (print_whole_file(fd) < 0)
                perror("print_whole_file");
            break;
        }
        if (got < 0) {
            perror("read");
            break;
        }
        if (got == 0)                /* конец ввода (Ctrl+D) */
            break;

        char *end;
        long n = strtol(input, &end, 10);
        if (end == input || (*end != '\n' && *end != '\0')) {
            printf("Нужно ввести целое число\n");
            continue;
        }
        if (n == 0)
            break;
        if (n < 0 || (size_t)n > table.count) {
            printf("Нет строки с номером %ld\n", n);
            continue;
        }

        if (print_line(fd, &table.items[n - 1]) < 0) {
            perror("read");
            break;
        }
    }

    free(table.items);
    close(fd);
    return EXIT_SUCCESS;
}