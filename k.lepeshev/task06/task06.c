#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define LIMIT 4096

typedef struct LineInfo{
    off_t offset;
    size_t length;
} LineInfo;

static volatile sig_atomic_t time_expired = 0;

void alarmHandler(int signal_number){
    (void)signal_number;

    int saved_errno = errno;

    time_expired = 1;
    close(STDIN_FILENO);

    errno = saved_errno;
}

int printWholeFile(int fd){

    if(fflush(stdout) == EOF){
        perror("fflush");
        return -1;
    }

    if(lseek(fd, 0, SEEK_SET) == -1){
        perror("lseek");
        return -1;
    }

    char buffer[LIMIT];

    ssize_t count = 0;
    ssize_t result = 0;
    size_t written = 0;

    while(1){
        count = read(fd, buffer, sizeof(buffer));

        if(count == 0)
            return 0;

        if(count == -1){
            if(errno == EINTR)
                continue;

            perror("read");
            return -1;
        }

        written = 0;

        while(written < (size_t)count){
            result = write(STDOUT_FILENO, buffer + written, (size_t)count - written);

            if(result == -1){
                if(errno == EINTR)
                    continue;

                perror("write");
                return -1;
            }

            if(result == 0){
                fprintf(stderr, "Couldn't write file\n");
                return -1;
            }

            written += (size_t)result;
        }

    }
}

int readLineNumber(long *number)
{
    char input[4096];

    if(fgets(input, sizeof input, stdin) == NULL)
        return -1;

    size_t length = strlen(input);

    if(length > 0 && input[length - 1] != '\n'){
        int next = getchar();

        if(next != '\n' && next != EOF){
            while((next = getchar()) != '\n' && next != EOF);

            return 0;
        }
    }

    if(ferror(stdin))
        return -1;

    char *end;
    errno = 0;

    long value = strtol(input, &end, 10);

    if(end == input || errno == ERANGE)
        return 0;

    while(isspace((unsigned char)*end))
        end++;

    if(*end != '\0')
        return 0;

    *number = value;
    return 1;
}

int main(void){
    struct sigaction action = {0};
    action.sa_handler = alarmHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    size_t lines_capacity = 2;

    LineInfo *lines = (LineInfo*)malloc(lines_capacity*sizeof(LineInfo));
    if(lines == NULL){
        perror("malloc");
        exit(1);
    }

    size_t idx = 0;

    int fd = open("text.txt", O_RDONLY);
    if(fd == -1){
        perror("open");
        free(lines);
        exit(1);
    }

    char ch;
    ssize_t result = 0;
    off_t line_start = 0;
    size_t line_lenght = 0;
    while((result = read(fd, &ch, 1)) > 0){
        line_lenght++;
        if(ch == '\n'){
            if(idx == lines_capacity){
                size_t new_capacity = lines_capacity * 2;
                LineInfo *new_lines = (LineInfo*)realloc(lines, new_capacity * sizeof(LineInfo));

                if(new_lines == NULL){
                    perror("realloc");
                    close(fd);
                    free(lines);
                    exit(1);
                }

                lines = new_lines;
                lines_capacity = new_capacity;
            }
            lines[idx].offset = line_start;
            lines[idx].length = line_lenght;
            idx++;

            line_start = lseek(fd, 0L, SEEK_CUR);
            if(line_start == (off_t)-1){
                perror("lseek");
                close(fd);
                free(lines);
                exit(1);
            }
            line_lenght = 0;
        }
    }

    if(result == -1){
        perror("read");
        close(fd);
        free(lines);
        exit(1);
    }

    if(line_lenght > 0){
        if(idx == lines_capacity){
                size_t new_capacity = lines_capacity * 2;
                LineInfo *new_lines = (LineInfo*)realloc(lines, new_capacity * sizeof(LineInfo));

                if(new_lines == NULL){
                    perror("realloc");
                    close(fd);
                    free(lines);
                    exit(1);
                }

                lines = new_lines;
                lines_capacity = new_capacity;
            }

        lines[idx].offset = line_start;
        lines[idx].length = line_lenght;
        idx++;
    }

    long number = 0;

    for(size_t i=0; i < idx; i++){
        printf("Line %zu: offset=%lld, length=%zu\n", i+1, (long long)lines[i].offset, lines[i].length);
    }

    int input_result = 0, print_result = 0;
    int start_timer = 1;

    while(1){
        printf("Enter line number (0 to exit): ");
        fflush(stdout);

        if(start_timer){
            time_expired = 0;
            alarm(5);
            start_timer = 0;
        }

        input_result = readLineNumber(&number);

        if(input_result == 1 && number >= 0 && (size_t)number <= idx){
            alarm(0);
        }

        if(time_expired){
            printf("\nTime's up. File contents:\n");

            print_result = printWholeFile(fd);

            close(fd);
            free(lines);

            return print_result == 0 ? 0 : 1;
        }

        if(input_result == -1)
            break;

        if(input_result == 0){
            printf("Invalid line number\n");
            continue;
        }

        if(number == 0)
            break;
        
        if(number < 0 || (size_t)number > idx){
            printf("No such line\n");
            continue;
        }

        size_t index = (size_t)(number - 1);

        off_t offset = lines[index].offset;
        size_t length = lines[index].length;

        if(lseek(fd, offset, SEEK_SET) == (off_t)-1){
            perror("lseek");
            break;
        }

        char *buffer = (char*)malloc(length + 1);
        if(buffer == NULL){
            perror("malloc");
            break;
        }
        ssize_t count = read(fd, buffer, length);
        if(count == -1){
            perror("read");
            free(buffer);
            break;
        }

        if((size_t)count != length){
            fprintf(stderr, "Couldn't read full line\n");
            free(buffer);
            break;
        }

        buffer[count] = '\0';

        printf("%s", buffer);

        if(count > 0 && buffer[count-1] != '\n')
            printf("\n");

        free(buffer);
        start_timer = 1;
    }

    alarm(0);

    close(fd);

    free(lines);

    exit(0);
}