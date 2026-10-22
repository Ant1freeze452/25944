#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h>   
#include <ctype.h>   

#define LIMIT 4096

typedef struct LineInfo{
    off_t offset;
    size_t length;
} LineInfo;


int main(void){
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

    while(1){
        printf("Enter line number (0 to exit): ");
        fflush(stdout);

        // if(scanf("%ld", &number) != 1)
        //     break;

        char input[LIMIT];

        if(fgets(input, sizeof(input), stdin) == NULL)
            break;

        size_t input_length = strlen(input);

        if(input_length > 0 && input[input_length - 1] != '\n'){
            int next = getchar();

            if(next != '\n' && next != EOF){
                while((next = getchar()) != '\n' && next != EOF);

                printf("Invalid line number\n");
                continue;
            }
        }

        char *end;
        errno = 0;

        number = strtol(input, &end, 10);

        if(end == input || errno == ERANGE){
            printf("Invalid line number\n");
            continue;
        }

        while(isspace((unsigned char)*end))
            end++;

        if(*end != '\0'){
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
    }

    close(fd);

    free(lines);

    exit(0);
}