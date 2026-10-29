#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

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

static int make_table(int fd, Line **table, size_t *count)
{
    off_t start = 0;
    size_t length = 0;
    char ch;
    ssize_t n;
    while ((n = read(fd, &ch, 1)) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        ++length;
        if (ch == '\n') {
            if (add_line(table, count, start, length - 1) < 0) return -1;
            start = lseek(fd, 0L, SEEK_CUR);
            if (start == (off_t)-1) return -1;
            length = 0;
        }
    }
    if (length != 0) return add_line(table, count, start, length);
    return 0;
}

static int print_text(int fd, off_t offset, size_t length)
{
    char buffer[1025];
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) return -1;
    while (length > 0) {
        size_t portion = length < 1024 ? length : 1024;
        ssize_t n = read(fd, buffer, portion);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) {
            if (n == 0) errno = EIO;
            return -1;
        }
        buffer[n] = '\0';
        printf("%s", buffer);
        length -= (size_t)n;
    }
    return 0;
}

static int read_input(char *input, size_t size)
{
    size_t used = 0;
    int overflow = 0;
    for (;;) {
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
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    Line *table = NULL;
    size_t count = 0;
    int status = 0;
    if (make_table(fd, &table, &count) < 0) {
        perror("make_table"); status = 1;
        goto cleanup;
    }
    off_t file_size = lseek(fd, 0L, SEEK_CUR);
    if (file_size == (off_t)-1) { perror("lseek"); status = 1; goto cleanup; }
    printf("line  offset  length (without newline)\n");
    for (size_t i = 0; i < count; ++i)
        printf("%zu  %jd  %zu\n", i + 1, (intmax_t)table[i].offset, table[i].length);
    for (;;) {
        char input[64], *end;
        printf("Line number (0 to exit): ");
        fflush(stdout);
        int result = read_input(input, sizeof(input));
        if (result == 0) break;
        if (result == -1) { perror("input"); status = 1; break; }
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
            if (print_text(fd, 0, (size_t)file_size) < 0) {
                perror("print_text"); status = 1;
            }
            break;
        }
        if (print_text(fd, table[number - 1].offset, table[number - 1].length) < 0) {
            perror("print_text"); status = 1; break;
        }
        printf("\n");
    }
cleanup:
    free(table);
    if (close(fd) < 0) { perror("close"); status = 1; }
    return status;
}
