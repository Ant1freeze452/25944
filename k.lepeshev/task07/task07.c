#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>

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

int printWholeFile(const char *file_data, size_t file_size){

    for(size_t i = 0; i < file_size; i++){
        if(putchar((unsigned char)file_data[i]) == EOF){
            perror("putchar");
            return -1;
        }
   }

    if(fflush(stdout) == EOF){
        perror("fflush");
        return -1;
    }

    return 0;
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

    struct stat file_info;

    if(fstat(fd, &file_info) == -1){
        perror("fstat");
        close(fd);
        free(lines);
        return 1;
    }

    size_t file_size = (size_t)file_info.st_size;

    if(file_size == 0){
        printf("File is empty\n");
        close(fd);
        free(lines);
        return 0;
    }

    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(file_data == MAP_FAILED){
        perror("mmap");
        close(fd);
        free(lines);
        return 1;
    }

    size_t line_start = 0;

    for(size_t i = 0; i < file_size; i++){
        if(file_data[i] == '\n'){
            if(idx == lines_capacity){
                size_t new_capacity = lines_capacity * 2;
                LineInfo *new_lines = (LineInfo*)realloc(lines, new_capacity * sizeof(LineInfo));

                if(new_lines == NULL){
                    perror("realloc");
                    munmap(file_data, file_size);
                    close(fd);
                    free(lines);
                    exit(1);
                }

                lines = new_lines;
                lines_capacity = new_capacity;
            }

            lines[idx].offset = (off_t)line_start;
            lines[idx].length = i - line_start + 1;
            idx++;

            line_start = i + 1;
        }
    }

    if(line_start < file_size){
        if(idx == lines_capacity){
                size_t new_capacity = lines_capacity * 2;
                LineInfo *new_lines = (LineInfo*)realloc(lines, new_capacity * sizeof(LineInfo));

                if(new_lines == NULL){
                    perror("realloc");
                    munmap(file_data, file_size);
                    close(fd);
                    free(lines);
                    exit(1);
                }

                lines = new_lines;
                lines_capacity = new_capacity;
            }

            lines[idx].offset = (off_t)line_start;
            lines[idx].length = file_size - line_start;
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

            print_result = printWholeFile(file_data, file_size);

            munmap(file_data, file_size);

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

        const char *line = file_data + offset;

        for(size_t i=0; i < length; i++){
            if(putchar((unsigned char)line[i]) == EOF){
                perror("putchar");
                munmap(file_data, file_size);
                close(fd);
                free(lines);
                exit(1);
            }
        }
        if(length > 0 && line[length - 1] != '\n'){
            if(putchar('\n') == EOF){
                perror("putchar");
                munmap(file_data, file_size);
                close(fd);
                free(lines);
                exit(1);
            }
        }

        start_timer = 1;
    }

    alarm(0);

    munmap(file_data, file_size);

    close(fd);

    free(lines);

    exit(0);
}