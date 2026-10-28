#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <ulimit.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct option {
    int name;
    char *argument;
};

extern char **environ;

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s -i -s -p -u -Ublocks "
            "-c -Cbytes -d -v -Vname=value\n", program);
}

static int number(const char *text, long *value)
{
    char *end;
    errno = 0;
    *value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || *value < 0) {
        fprintf(stderr, "Invalid number: %s\n", text);
        return -1;
    }
    return 0;
}

static int execute(struct option option)
{
    struct rlimit limit;
    long value, result;
    char path[4096];
    char **variable;

    switch (option.name) {
    case 'i':
        printf("UID: %lu\nEUID: %lu\nGID: %lu\nEGID: %lu\n",
               (unsigned long)getuid(), (unsigned long)geteuid(),
               (unsigned long)getgid(), (unsigned long)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            return -1;
        }
        break;
    case 'p':
        printf("PID: %ld\nPPID: %ld\nPGRP: %ld\n",
               (long)getpid(), (long)getppid(), (long)getpgrp());
        break;
    case 'u':
        errno = 0;
        result = ulimit(UL_GETFSIZE);
        if (result == -1 && errno != 0) {
            perror("ulimit");
            return -1;
        }
        printf("ulimit: %ld (512-byte blocks)\n", result);
        break;
    case 'U':
        if (number(option.argument, &value) == -1)
            return -1;
        errno = 0;
        result = ulimit(UL_SETFSIZE, value);
        if (result == -1 && errno != 0) {
            perror("ulimit");
            return -1;
        }
        break;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("getrlimit");
            return -1;
        }
        if (limit.rlim_cur == RLIM_INFINITY)
            puts("core limit: unlimited");
        else
            printf("core limit: %llu bytes\n",
                   (unsigned long long)limit.rlim_cur);
        break;
    case 'C':
        if (number(option.argument, &value) == -1)
            return -1;
        if (getrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("getrlimit");
            return -1;
        }
        limit.rlim_cur = (rlim_t)value;
        if (setrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("setrlimit");
            return -1;
        }
        break;
    case 'd':
        if (getcwd(path, sizeof(path)) == NULL) {
            perror("getcwd");
            return -1;
        }
        puts(path);
        break;
    case 'v':
        for (variable = environ; *variable != NULL; ++variable)
            puts(*variable);
        break;
    case 'V':
        if (strchr(option.argument, '=') == NULL || option.argument[0] == '=') {
            fprintf(stderr, "Expected -V name=value\n");
            return -1;
        }
        if (putenv(option.argument) != 0) {
            perror("putenv");
            return -1;
        }
        break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct option *options = NULL;
    int count = 0, opt, status = 0;

    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }
    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        struct option *grown;
        if (opt == '?') {
            usage(argv[0]);
            free(options);
            return 1;
        }
        grown = realloc(options, (count + 1) * sizeof(*options));
        if (grown == NULL) {
            perror("realloc");
            free(options);
            return 1;
        }
        options = grown;
        options[count].name = opt;
        options[count].argument = optarg;
        ++count;
    }
    if (optind != argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        usage(argv[0]);
        free(options);
        return 1;
    }
    for (int i = count - 1; i >= 0; --i) {
        if (execute(options[i]) == -1)
            status = 1;
    }
    free(options);
    return status;
}
