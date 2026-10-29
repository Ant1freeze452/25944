#define _XOPEN_SOURCE 700
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    int option;
    char *argument;
} Operation;

static int read_number(const char *text, long *value)
{
    char *end;
    errno = 0;
    *value = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || *value < 0) {
        fprintf(stderr, "Invalid size: %s\n", text);
        return 1;
    }
    return 0;
}

static int execute(Operation operation)
{
    long value;
    struct rlimit limit;
    char directory[PATH_MAX];

    switch (operation.option) {
    case 'i':
        printf("UID: %lu, EUID: %lu, GID: %lu, EGID: %lu\n",
            (unsigned long)getuid(), (unsigned long)geteuid(),
            (unsigned long)getgid(), (unsigned long)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            return 1;
        }
        break;
    case 'p':
        printf("PID: %ld, PPID: %ld, PGID: %ld\n",
            (long)getpid(), (long)getppid(), (long)getpgrp());
        break;
    case 'u':
#ifdef RLIMIT_NPROC
        if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
            perror("getrlimit");
            return 1;
        }
        if (limit.rlim_cur == RLIM_INFINITY) {
            printf("ulimit: unlimited processes\n");
        } else {
            printf("ulimit: %ju processes\n", (uintmax_t)limit.rlim_cur);
        }
#else
        errno = 0;
        value = sysconf(_SC_CHILD_MAX);
        if (value == -1 && errno != 0) {
            perror("sysconf");
            return 1;
        }
        if (value == -1) {
            printf("ulimit: unspecified process limit\n");
        } else {
            printf("ulimit: %ld processes\n", value);
        }
#endif
        break;
    case 'U':
        if (read_number(operation.argument, &value)) {
            return 1;
        }
#ifdef RLIMIT_NPROC
        if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
            perror("getrlimit");
            return 1;
        }
        limit.rlim_cur = (rlim_t)value;
        if (setrlimit(RLIMIT_NPROC, &limit) == -1) {
            perror("setrlimit");
            return 1;
        }
#else
        fprintf(stderr, "Changing the per-user process limit is not supported on this system.\n");
        return 1;
#endif
        break;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("getrlimit");
            return 1;
        }
        if (limit.rlim_cur == RLIM_INFINITY) {
            printf("Core size: unlimited\n");
        } else {
            printf("Core size: %ju bytes\n", (uintmax_t)limit.rlim_cur);
        }
        break;
    case 'C':
        if (read_number(operation.argument, &value)) {
            return 1;
        }
        if (getrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("getrlimit");
            return 1;
        }
        limit.rlim_cur = (rlim_t)value;
        if (setrlimit(RLIMIT_CORE, &limit) == -1) {
            perror("setrlimit");
            return 1;
        }
        break;
    case 'd':
        if (getcwd(directory, sizeof(directory)) == NULL) {
            perror("getcwd");
            return 1;
        }
        printf("%s\n", directory);
        break;
    case 'v':
        for (char **entry = environ; *entry != NULL; ++entry) {
            puts(*entry);
        }
        break;
    case 'V': {
        char *equal = strchr(operation.argument, '=');
        if (equal == NULL || equal == operation.argument) {
            fprintf(stderr, "Expected name=value: %s\n", operation.argument);
            return 1;
        }
        if (putenv(operation.argument) != 0) {
            perror("putenv");
            return 1;
        }
        break;
    }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    size_t capacity = 0;
    for (int i = 1; i < argc; ++i) {
        capacity += strlen(argv[i]);
    }
    if (capacity == 0) {
        printf("Usage: %s [options]\n", argv[0]);
        puts("Options are executed from right to left.\n"
            "  -i            Show real and effective user/group IDs\n"
            "  -s            Become a process group leader\n"
            "  -p            Show process, parent and process group IDs\n"
            "  -u            Show the process count limit\n"
            "  -U<number>    Change the process count limit (if supported)\n"
            "  -c            Show the core file limit in bytes\n"
            "  -C<size>      Change the core file limit in bytes\n"
            "  -d            Show the current working directory\n"
            "  -v            Show environment variables\n"
            "  -Vname=value  Set an environment variable");
#ifndef RLIMIT_NPROC
        puts("Note: -U is not supported on this system.");
#endif
        return 0;
    }

    Operation *operations = malloc(capacity * sizeof(*operations));
    if (operations == NULL) {
        perror("malloc");
        return 1;
    }

    size_t count = 0;
    int option;
    opterr = 0;
    while ((option = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (option == '?' || option == ':') {
            fprintf(stderr, "%s: -%c\n",
                    option == '?' ? "Unknown option" : "Missing argument",
                    optopt);
            free(operations);
            return 1;
        }
        operations[count].option = option;
        operations[count].argument = optarg;
        ++count;
    }

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        free(operations);
        return 1;
    }

    int result = 0;
    while (count > 0) {
        result = execute(operations[--count]);
        if (result != 0) {
            break;
        }
    }
    free(operations);
    return result;
}
