#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    off_t offset;
    size_t length;
} Line;

static int add_line(Line **table, size_t *count, off_t offset, size_t length)
{
    Line *next = realloc(*table, (*count + 1) * sizeof(Line));
    if (next == NULL) return -1;
    *table = next;
    (*table)[*count].offset = offset;
    (*table)[*count].length = length;
    ++*count;
    return 0;
}

static int make_table(const char *data, size_t size, Line **table, size_t *count)
{
    size_t start = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            if (add_line(table, count, (off_t)start, i - start) < 0) return -1;
            start = i + 1;
        }
    }
    if (start < size) return add_line(table, count, (off_t)start, size - start);
    return 0;
}

static int print_text(const char *data, off_t offset, size_t length)
{
    while (length > 0) {
        size_t portion = length < 1024 ? length : 1024;
        printf("%.*s", (int)portion, data + offset);
        offset += (off_t)portion;
        length -= portion;
    }
    return 0;
}

static volatile sig_atomic_t timed_out = 0;
static sigset_t wait_mask;

static void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

static int read_input(char *input, size_t size, int first)
{
    size_t used = 0;
    int overflow = 0;
    for (;;) {
        if (first) {
            if (timed_out) return 2;
            fd_set ready;
            FD_ZERO(&ready);
            FD_SET(STDIN_FILENO, &ready);
            int n = pselect(STDIN_FILENO + 1, &ready, NULL, NULL, NULL, &wait_mask);
            if (n < 0) {
                if (errno == EINTR) continue;
                return -1;
            }
            if (timed_out) return 2;
        }
        int ch = getchar();
        if (ch == EOF) {
            if (ferror(stdin)) return -1;
            if (used == 0 && !overflow) return 0;
            break;
        }
        if (ch == '\n') break;
        if (used + 1 < size && ch != '\0') input[used++] = (char)ch;
        else overflow = 1;
    }
    input[used] = '\0';
    return overflow ? 3 : 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }
    struct sigaction action = {0};
    sigset_t blocked, old_mask;
    action.sa_handler = on_alarm;
    sigemptyset(&action.sa_mask);
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    if (sigaction(SIGALRM, &action, NULL) < 0 ||
        sigprocmask(SIG_BLOCK, &blocked, &old_mask) < 0) {
        perror("signal setup");
        return 1;
    }
    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGALRM);
    setvbuf(stdin, NULL, _IONBF, 0);
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    struct stat info;
    if (fstat(fd, &info) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }
    size_t file_size = (size_t)info.st_size;
    char *data = NULL;
    if (file_size > 0) {
        data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }
    Line *table = NULL;
    size_t count = 0;
    int status = 0;
    if (make_table(data, file_size, &table, &count) < 0) {
        perror("make_table"); status = 1;
        goto cleanup;
    }
    printf("line  offset  length (without newline)\n");
    for (size_t i = 0; i < count; ++i)
        printf("%zu  %jd  %zu\n", i + 1, (intmax_t)table[i].offset, table[i].length);
    int first = 1;
    for (;;) {
        char input[64], *end;
        printf(first ? "Line number (0 to exit, 5 seconds): " : "Line number (0 to exit): ");
        fflush(stdout);
        if (first) {
            timed_out = 0;
            alarm(5);
        }
        int result = read_input(input, sizeof(input), first);
        if (result == 2) {
            printf("\nTime is up. Full file:\n");
            if (print_text(data, 0, file_size) < 0) {
                perror("print_text"); status = 1;
            }
            break;
        }
        if (result == 0) break;
        if (result == -1) { perror("input"); status = 1; break; }
        if (first) {
            alarm(0);
            first = 0;
            sigprocmask(SIG_SETMASK, &old_mask, NULL);
        }
        if (result == 3) { printf("Invalid line number\n"); continue; }
        errno = 0;
        long number = strtol(input, &end, 10);
        while (end != input && (*end == ' ' || *end == '\t' || *end == '\r')) ++end;
        if (errno == ERANGE || end == input || *end != '\0' ||
            number < 0 || (uintmax_t)number > count) {
            printf("Invalid line number\n");
            continue;
        }
        if (number == 0) {
            printf("Full file:\n");
            if (print_text(data, 0, file_size) < 0) {
                perror("print_text"); status = 1;
            }
            break;
        }
        if (print_text(data, table[number - 1].offset, table[number - 1].length) < 0) {
            perror("print_text"); status = 1; break;
        }
        printf("\n");
    }
cleanup:
    alarm(0);
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    if (data != NULL && munmap(data, file_size) < 0) {
        perror("munmap"); status = 1;
    }
    free(table);
    if (close(fd) < 0) { perror("close"); status = 1; }
    return status;
}
