#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/resource.h>
#include <ulimit.h>

extern char **environ;

typedef struct Option{
    char name;
    char *argument;
} Option;

typedef struct rlimit rlimit;


int main(int argc, char *argv[]){

    if(argc == 1){
        printf("Usage: %s [-i] [-s] [-p] [-u] [-Ulimit] [-c] [-Csize] [-d] [-v] [-Vname=value]\n", argv[0]);
        return 0;
    }

    size_t capacity = 1;

    for(int i=1; i < argc; i++)
        capacity += strlen(argv[i]);

    Option *options = (Option*)malloc(capacity*sizeof(Option));

    if(options == NULL){
        perror("malloc");
        return 1;
    }


    int idx = 0;

    int option = 0;

    while((option = getopt(argc, argv, "ipdsuU:cC:vV:")) != -1){
        switch(option){
            case 'i':
            case 'p':
            case 'd':
            case 's':
            case 'u':
            case 'c':
            case 'v':
                options[idx].name = option;
                options[idx].argument = NULL;
                idx++;
                break;
            case 'U':
            case 'C':
            case 'V':
                options[idx].name = option;
                options[idx].argument = optarg;
                idx++;
                break;
            case '?':
                free(options);
                return 1;

        }
    }

    for(int i=idx-1; i >= 0; i--){
        switch(options[i].name){
            case 'i':
                printf("Real UID: %lu\n", (unsigned long)getuid());
                printf("Effective UID: %lu\n", (unsigned long)geteuid());
                printf("Real GID: %lu\n", (unsigned long)getgid());
                printf("Effective GID: %lu\n", (unsigned long)getegid());
                break;
            case 'p':
                printf("Process ID: %ld\n", (long)getpid());
                printf("Parent process ID: %ld\n", (long)getppid());
                printf("Group process ID: %ld\n", (long)getpgrp());
                break;
            case 'd':
            {
                char path[4096];

                if(getcwd(path, sizeof(path)) != NULL)
                    printf("Current directory: %s\n", path);
                else
                    perror("getcwd");

                break;
            }
            case 's':
                if(setpgid(0, 0) == -1)
                    perror("setpgid");
                break;
            case 'u':
            {
                errno = 0;
                long limit = sysconf(_SC_CHILD_MAX);

                if(limit == -1){
                    if(errno != 0)
                        perror("ulimit");
                    else{
                        printf("unlimited\n");
                    }
                }
                else{
                    printf("Ulimit: %ld\n", limit);
                }

                break;
            }
            case 'c':
            {
                rlimit limits;

                if(getrlimit(RLIMIT_CORE, &limits) == -1){
                    perror("getrlimit");
                    break;
                }

                if(limits.rlim_cur == RLIM_INFINITY)
                    printf("Core file size limit: unlimited\n");
                else
                    printf("Core file size limit: %llu bytes\n", (unsigned long long)limits.rlim_cur);
                
                break;
            }
            case 'v':
            {
                for(char **env = environ; *env != NULL; env++)
                    printf("%s\n", *env);
                    
                break;
            }
            case 'U':
            {
                char *endptr;
                
                errno = 0;

                long new_limit = strtol(options[i].argument, &endptr, 10);

                if(endptr == options[i].argument || *endptr != '\0' || errno == ERANGE || new_limit < 0){
                    fprintf(stderr, "Invalid ulimit value\n");
                    break;
                }

                long result = ulimit(UL_SETFSIZE, new_limit);

                if(result == -1)
                    perror("ulimit");

                break;
            }
            case 'C':
            {
                char *endptr;

                if(options[i].argument[0] == '-'){
                    fprintf(stderr, "Invalid core file size\n");
                    break;
                }

                errno = 0;

                unsigned long long new_size = strtoull(options[i].argument, &endptr, 10);

                if(endptr == options[i].argument || *endptr != '\0' || errno == ERANGE){
                    fprintf(stderr, "Invalid core size value\n");
                    break;
                }

                rlimit limits;

                if(getrlimit(RLIMIT_CORE, &limits) == -1){
                    perror("getrlimit");
                    break;
                }

                rlim_t converted_size = (rlim_t)new_size;

                if((unsigned long long)converted_size != new_size || converted_size == RLIM_INFINITY){
                    fprintf(stderr, "Invalid core size value\n");
                    break;
                }

                limits.rlim_cur = converted_size;

                if(setrlimit(RLIMIT_CORE, &limits) == -1)
                    perror("setrlimit");
        
                break;
            }
            case 'V':
            {
                char *argument = options[i].argument;
                char *separator = strchr(argument, '=');

                if(separator == NULL || separator == argument){
                    fprintf(stderr, "Invalid enviroment parametrs\n");
                    break;
                }

                size_t name_length = (size_t)(separator - argument);

                char *name = (char*)malloc(name_length + 1);

                if(name == NULL){
                    perror("malloc");
                    break;
                }

                memcpy(name, argument, name_length);
                name[name_length] = '\0';

                char *value = separator + 1;

                if(setenv(name, value, 1) == -1){
                    perror("setenv");
                }

                free(name);

                break;
            }

        }
    }

    free(options);

    return 0;
}