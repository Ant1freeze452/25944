#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TIMEOUT 5

static off_t *offsets = NULL; 
static long *lengths = NULL;    
static long count = 0, capacity = 0;

static volatile sig_atomic_t timed_out = 0;

static void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

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
    struct stat st;
    char *data = NULL;       
    off_t size;
    off_t line_start = 0;       
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

    // размер файла
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return 1;
    }
    size = st.st_size;

    if (size > 0) {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            perror("mmap");
            return 1;
        }
    }
    close(fd);               

    for (off_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            add_line(line_start, i - line_start);
            line_start = i + 1;
        }
    }

    if (size > line_start)
        add_line(line_start, size - line_start);

    printf("строка  отступ  длина\n");
    for (long i = 0; i < count; i++)
        printf("%6ld  %6lld  %5ld\n", i + 1, (long long)offsets[i], lengths[i]);

    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // таймер только на первый ввод
    for (int first = 1;; first = 0) {
        char input[256];
        char *end;
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
            if (size > 0)
                printf("%.*s", (int)size, data);
            break;
        }
        if (got == NULL)
            break;

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

        printf("%.*s\n", (int)lengths[number - 1], data + offsets[number - 1]);
    }

    if (data != NULL)
        munmap(data, size);
    free(offsets);
    free(lengths);
    return 0;
}