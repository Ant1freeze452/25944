#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>

#define MAX_OPTIONS 256

extern char *optarg;
extern char **environ;

typedef struct {
    int option;
    char *argument;
} OptionInfo;


static void print_usage(const char *program)
{
    printf("Usage: %s\n"
           "  -i             Print user and group IDs\n"
           "  -s             Set process group ID\n"
           "  -p             Print process IDs\n"
           "  -u             Print ulimit\n"
           "  -U value       Change ulimit\n"
           "  -c             Print core file size\n"
           "  -C size        Change core file size\n"
           "  -d             Print current directory\n"
           "  -v             Print environment variables\n"
           "  -V name=value  Set environment variable\n",
           program);
}


static int parse_nonnegative_long(const char *str, long *result)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(str, &end, 10);

    if (end == str || *end != '\0' || errno != 0 || value < 0) {
        return -1;
    }

    *result = value;
    return 0;
}


static long get_process_limit(void)
{
#ifdef RLIMIT_NPROC
    struct rlimit limit;

    if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
        if (limit.rlim_cur == RLIM_INFINITY) {
            return -2;
        }

        return (long)limit.rlim_cur;
    }
#endif

    return sysconf(_SC_CHILD_MAX);
}


static long get_core_limit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        return -1;
    }

    if (limit.rlim_cur == RLIM_INFINITY) {
        return -2;
    }

    return (long)(limit.rlim_cur / 1024);
}


static void print_process_limit(void)
{
    long value = get_process_limit();

    if (value == -2) {
        printf("unlimited\n");
    } else if (value < 0) {
        perror("ulimit -u");
    } else {
        printf("%ld\n", value);
    }
}


static void print_core_limit(void)
{
    long value = get_core_limit();

    if (value == -2) {
        printf("unlimited\n");
    } else if (value < 0) {
        perror("ulimit -c");
    } else {
        printf("%ld\n", value);
    }
}


int main(int argc, char *argv[])
{
    OptionInfo options[MAX_OPTIONS];
    int option_count = 0;
    int option;

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    opterr = 0;

    while ((option = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (option == '?') {
            fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
            print_usage(argv[0]);
            return 1;
        }

        if (option == ':') {
            fprintf(stderr, "Option -%c requires an argument\n", optopt);
            print_usage(argv[0]);
            return 1;
        }

        if (option_count < MAX_OPTIONS) {
            options[option_count].option = option;
            options[option_count].argument = optarg;
            option_count++;
        } else {
            fprintf(stderr, "Maximum number of options reached: %d\n",
                    MAX_OPTIONS);
            return 1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        print_usage(argv[0]);
        return 1;
    }

    for (int i = option_count - 1; i >= 0; i--) {
        switch (options[i].option) {
            case 'i':
                printf("Real UID: %d, Effective UID: %d\n",
                       getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n",
                       getgid(), getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1) {
                    perror("setpgid");
                } else {
                    printf("New PGID: %d\n", (int)getpgrp());
                }
                break;

            case 'p':
                printf("PID: %d, PPID: %d, PGID: %d\n",
                       (int)getpid(),
                       (int)getppid(),
                       (int)getpgrp());
                break;

            case 'u':
                print_process_limit();
                break;

            case 'U': {
                long value;

                if (parse_nonnegative_long(options[i].argument, &value) == -1) {
                    printf("Invalid value\n");
                    break;
                }

#ifdef RLIMIT_NPROC
                {
                    struct rlimit limit;

                    if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
                        limit.rlim_cur = (rlim_t)value;

                        if (setrlimit(RLIMIT_NPROC, &limit) == -1) {
                            perror("setrlimit");
                        }
                    }
                }
#endif
                printf("%ld\n", value);
                break;
            }

            case 'c':
                print_core_limit();
                break;

            case 'C': {
                struct rlimit limit;
                long value;

                if (parse_nonnegative_long(options[i].argument, &value) == -1) {
                    printf("Invalid value\n");
                    break;
                }

                if (getrlimit(RLIMIT_CORE, &limit) == 0) {
                    limit.rlim_cur = (rlim_t)(value * 1024);

                    if (setrlimit(RLIMIT_CORE, &limit) == 0) {
                        printf("%ld\n", value);
                    } else {
                        perror("setrlimit");
                    }
                }

                break;
            }

            case 'd': {
                char cwd[1024];

                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("Current directory: %s\n", cwd);
                } else {
                    perror("getcwd");
                }

                break;
            }

            case 'v': {
                char **env;

                for (env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }

                break;
            }

            case 'V':
                if (putenv(options[i].argument) == 0) {
                    printf("Environment updated: %s\n", options[i].argument);
                } else {
                    perror("putenv");
                }
                break;
        }
    }

    return 0;
}
