#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int check_file(const char *path)
{
    FILE *file;
    printf("UID: %ju, EUID: %ju\n",
           (uintmax_t)getuid(), (uintmax_t)geteuid());
    fflush(stdout);
    file = fopen(path, "r+");
    if (file == NULL) {
        perror(path);
        return -1;
    }
    puts("fopen(r+): success");
    if (fclose(file) == EOF) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int first, second;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s data-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    puts("Before setuid(getuid()):");
    first = check_file(argv[1]);
    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }
    puts("After setuid(getuid()):");
    second = check_file(argv[1]);
    return first == 0 && second == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
