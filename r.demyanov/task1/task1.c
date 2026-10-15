#define _XOPEN_SOURCE 600

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

extern char **environ;

// синтаксис вызова
static void usage(const char *name)
{
    fprintf(stderr,
            "Использование: %s [-i] [-s] [-p] [-u] [-U новый_ulimit] [-c] [-C размер]\n"
            "       [-d] [-v] [-V имя=значение]\n"
            "  -i  печать uid, euid, gid, egid\n"
            "  -s  процесс становится лидером группы\n"
            "  -p  печать pid, ppid и pgrp\n"
            "  -u  печать лимита количества процессов\n"
            "  -U  изменение лимита количества процессов\n"
            "  -c  печать размера core-файла\n"
            "  -C  изменение размера core-файла\n"
            "  -d  печать текущей директории\n"
            "  -v  печать переменных среды\n"
            "  -V  добавление или изменение переменной среды\n"
            "Опции обрабатываются справа налево.\n",
            name);
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        usage(argv[0]);
        return 1;
    }

    size_t max_options = 1;

    // считаем максимальное количество опций
    for (int i = 1; i < argc; i++) {
        size_t length = strlen(argv[i]);
        if (length > SIZE_MAX - max_options) {
            fprintf(stderr, "Слишком много аргументов\n");
            return 1;
        }
        max_options += length;
    }

    if (max_options > SIZE_MAX / sizeof(int) ||
        max_options > SIZE_MAX / sizeof(char *)) {
        fprintf(stderr, "Слишком много аргументов\n");
        return 1;
    }
    int *options = malloc(max_options * sizeof(*options));
    char **values = malloc(max_options * sizeof(*values));
    if (options == NULL || values == NULL) {
        fprintf(stderr, "Недостаточно памяти\n");
        free(options);
        free(values);
        return 1;
    }
    size_t count = 0;
    int error = 0;
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (c == '?') {
            fprintf(stderr, "Неизвестная опция: -%c\n", optopt);
            usage(argv[0]);
            free(options);
            free(values);
            return 1;
        }
        if (c == ':') {
            fprintf(stderr, "Опции -%c нужен аргумент\n", optopt);
            usage(argv[0]);
            free(options);
            free(values);
            return 1;
        }

        options[count] = c;
        values[count] = optarg;
        count++;
    }

    // лишние аргументы, не являющиеся опциями
    if (optind < argc) {
        fprintf(stderr, "Неизвестный аргумент: %s\n", argv[optind]);
        usage(argv[0]);
        free(options);
        free(values);
        return 1;
    }

    // обрабатываем опции справа налево
    while (count > 0) {
        size_t i = --count;
        struct rlimit limit;
        char *end;
        long number;

        switch (options[i]) {
        // печать идентификаторов пользователя и группы
        case 'i':
            printf("uid=%lu euid=%lu gid=%lu egid=%lu\n",
                   (unsigned long)getuid(), (unsigned long)geteuid(),
                   (unsigned long)getgid(), (unsigned long)getegid());
            break;

        // процесс становится лидером группы
        case 's':
            if (setpgid(0, 0) == -1) {
                perror("setpgid");
                error = 1;
            }
            break;

        // печать идентификаторов процесса родителя и группы процессов
        case 'p':
            printf("pid=%ld ppid=%ld pgrp=%ld\n",
                   (long)getpid(), (long)getppid(), (long)getpgrp());
            break;

        // печать лимита количества процессов
        case 'u': {
            long process_limit;

#ifdef RLIMIT_NPROC
            if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
                if (limit.rlim_cur == RLIM_INFINITY)
                    printf("unlimited\n");
                else
                    printf("%llu\n", (unsigned long long)limit.rlim_cur);
                break;
            }
#endif
            errno = 0;
            process_limit = sysconf(_SC_CHILD_MAX);
            if (process_limit == -1) {
                if (errno != 0) {
                    perror("sysconf");
                    error = 1;
                } else {
                    printf("unlimited\n");
                }
            } else {
                printf("%ld\n", process_limit);
            }
            break;
        }

        // изменение лимита, если RLIMIT_NPROC доступен
        case 'U':
            errno = 0;
            number = strtol(values[i], &end, 10);
            if (end == values[i] || *end != '\0' || errno != 0 || number < 0) {
                fprintf(stderr, "Неверное значение для -U: %s\n", values[i]);
                error = 1;
                break;
            }
#ifdef RLIMIT_NPROC
            if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
                perror("getrlimit");
                error = 1;
            } else if ((uintmax_t)number >= (uintmax_t)RLIM_INFINITY ||
                       (limit.rlim_max != RLIM_INFINITY &&
                        (uintmax_t)number > (uintmax_t)limit.rlim_max)) {
                fprintf(stderr, "Слишком большое значение для -U\n");
                error = 1;
            } else {
                limit.rlim_cur = (rlim_t)number;
                if (setrlimit(RLIMIT_NPROC, &limit) == -1) {
                    perror("setrlimit");
                    error = 1;
                }
            }
#endif
            printf("%ld\n", number);
            break;

        // печать размера core файла
        case 'c':
            if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                perror("getrlimit");
                error = 1;
            } else if (limit.rlim_cur == RLIM_INFINITY) {
                printf("core=unlimited\n");
            } else {
                printf("core=%llu\n", (unsigned long long)limit.rlim_cur);
            }
            break;

        // изменение размера core файла
        case 'C':
            errno = 0;
            number = strtol(values[i], &end, 10);
            if (end == values[i] || *end != '\0' || errno != 0 || number < 0) {
                printf("Неверное значение для -C: %s\n", values[i]);
                error = 1;
                break;
            }
            if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                perror("getrlimit");
                error = 1;
                break;
            }
            if ((uintmax_t)number >= (uintmax_t)RLIM_INFINITY ||
                (limit.rlim_max != RLIM_INFINITY &&
                 (uintmax_t)number > (uintmax_t)limit.rlim_max)) {
                fprintf(stderr, "Слишком большое значение для -C\n");
                error = 1;
                break;
            }
            limit.rlim_cur = (rlim_t)number;
            if (setrlimit(RLIMIT_CORE, &limit) == -1) {
                perror("setrlimit");
                error = 1;
            }
            break;

        // печать текущей рабочей директории
        case 'd': {
            char directory[4096];

            if (getcwd(directory, sizeof(directory)) == NULL) {
                perror("getcwd");
                error = 1;
            } else {
                printf("%s\n", directory);
            }
            break;
        }

        // печать всех переменных среды
        case 'v':
            for (int j = 0; environ[j] != NULL; j++)
                printf("%s\n", environ[j]);
            break;

        // добавление или изменение переменной среды
        case 'V':
            if (strchr(values[i], '=') == NULL || values[i][0] == '=') {
                printf("Для -V требуется имя=значение\n");
                error = 1;
            } else if (putenv(values[i]) == -1) {
                perror("putenv");
                error = 1;
            }
            break;
        }
    }

    free(options);
    free(values);
    return error;
}
