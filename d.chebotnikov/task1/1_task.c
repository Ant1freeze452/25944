#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ulimit.h>

extern char **environ;

static void print_help(const char *program)
{
    printf("Usage: %s options\n", program);
    puts("Available options:");
    puts("  -i             print user and group identifiers");
    puts("  -s             make the process a process-group leader");
    puts("  -p             print process identifiers");
    puts("  -u             print the current ulimit value");
    puts("  -Unew_ulimit   set a new ulimit value");
    puts("  -c             print the maximum core-file size");
    puts("  -Csize         set the maximum core-file size");
    puts("  -d             print the current working directory");
    puts("  -v             print environment variables");
    puts("  -Vname=value   add or change an environment variable");
}

static int number(char *s, long *n)
{
    char *end;
    errno = 0;
    *n = strtol(s, &end, 10);
    return *s && *s != '-' && !*end && errno != ERANGE;
}

static int options(int argc, char **argv)
{
    int c = getopt(argc, argv, ":ispuU:cC:dvV:");
    char *arg = optarg;
    struct rlimit r;
    char path[PATH_MAX + 1], **p;
    long n;

    if (c == -1) {
        if (optind < argc)
            fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return optind < argc ? -1 : 0;
    }
    if (c == '?' || c == ':') {
        fprintf(stderr, c == '?' ? "Unknown option: -%c\n" :
            "Option -%c needs an argument\n", optopt);
        return -1;
    }

    if (options(argc, argv) == -1)
        return -1;

    switch (c) {
    case 'i':
        printf("uid=%lu euid=%lu gid=%lu egid=%lu\n",
            (unsigned long)getuid(), (unsigned long)geteuid(),
            (unsigned long)getgid(), (unsigned long)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1) { perror("setpgid"); return -1; }
        break;
    case 'p':
        printf("pid=%ld ppid=%ld pgrp=%ld\n",
            (long)getpid(), (long)getppid(), (long)getpgrp());
        break;
    case 'u':
        errno = 0;
        n = ulimit(UL_GETFSIZE);
        if (n == -1 && errno) { perror("ulimit"); return -1; }
        printf("ulimit=%ld\n", n);
        break;
    case 'U':
        if (!number(arg, &n)) { fprintf(stderr, "Bad ulimit: %s\n", arg); return -1; }
        if (ulimit(UL_SETFSIZE, n) == -1) { perror("ulimit"); return -1; }
        break;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &r) == -1) { perror("getrlimit"); return -1; }
        if (r.rlim_cur == RLIM_INFINITY) puts("core=unlimited");
        else printf("core=%llu\n", (unsigned long long)r.rlim_cur);
        break;
    case 'C':
        if (!number(arg, &n)) { fprintf(stderr, "Bad core size: %s\n", arg); return -1; }
        if (getrlimit(RLIMIT_CORE, &r) == -1) { perror("getrlimit"); return -1; }
        r.rlim_cur = (rlim_t)n;
        if (setrlimit(RLIMIT_CORE, &r) == -1) { perror("setrlimit"); return -1; }
        break;
    case 'd':
        if (!getcwd(path, sizeof path)) { perror("getcwd"); return -1; }
        puts(path);
        break;
    case 'v':
        for (p = environ; *p; ++p) puts(*p);
        break;
    case 'V':
        if (!strchr(arg, '=') || *arg == '=') {
            fprintf(stderr, "Expected name=value: %s\n", arg); return -1;
        }
        if (putenv(arg)) { perror("putenv"); return -1; }
        break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
    }

    opterr = 0;
    return options(argc, argv) == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
