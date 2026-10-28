#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int show_and_open(const char *name)
{
    FILE *file;

    printf("real uid = %lu, effective uid = %lu\n",
        (unsigned long)getuid(), (unsigned long)geteuid());

    file = fopen(name, "r+");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    puts("fopen: success");
    if (fclose(file) == EOF) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int status = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    puts("Before setuid:");
    if (show_and_open(argv[1]) == -1)
        status = EXIT_FAILURE;

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    puts("After setuid:");
    if (show_and_open(argv[1]) == -1)
        status = EXIT_FAILURE;

    return status;
}
