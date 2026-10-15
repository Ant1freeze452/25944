#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 4096

typedef struct {
    off_t offset;
    int length;
} LineInfo;

static int fd_global;
static ssize_t total_size_global;
static char buffer_global[BUFFER_SIZE];

void alarm_handler(int sig) {
    (void)sig;
    ssize_t bytes_read;
    
    printf("\nTime is over, printing entire file:\n");
    
    lseek(fd_global, 0L, SEEK_SET);
    while ((bytes_read = read(fd_global, buffer_global, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer_global, bytes_read);
    }
    
    close(fd_global);
    exit(0);
}

int main(int argc, char *argv[]) {
    const char *filename;
    LineInfo lines[MAX_LINES];
    int line_count = 1;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    off_t file_pos = 0;
    int i;
    int line_num;
    off_t pos;
    struct sigaction sa;
    int first_request = 1;

    if (argc < 2) {
        filename = "test.txt";
    } else {
        filename = argv[1];
    }

    fd_global = open(filename, O_RDONLY);
    if (fd_global < 0) {
        perror("Error opening file");
        return 1;
    }

    total_size_global = lseek(fd_global, 0L, SEEK_END);
    lseek(fd_global, 0L, SEEK_SET);

    lines[0].offset = 0;
    lines[0].length = 0;
    line_count = 1;

    while ((bytes_read = read(fd_global, buffer, BUFFER_SIZE)) > 0) {
        int j;
        for (j = 0; j < bytes_read && line_count < MAX_LINES; j++) {
            off_t current_pos = file_pos + j;
            if (buffer[j] == '\n') {
                lines[line_count - 1].length = current_pos - lines[line_count - 1].offset;
                lines[line_count].offset = current_pos + 1;
                lines[line_count].length = 0;
                line_count++;
            }
        }
        file_pos += bytes_read;
    }

    if (line_count == 1 && lines[0].length == 0 && total_size_global == 0) {
    } 
    else if (lines[line_count - 1].offset < total_size_global) {
        lines[line_count - 1].length = total_size_global - lines[line_count - 1].offset;
    }

    printf("Table:\n");
    for (i = 0; i < line_count; i++) {
        printf("Line %d: offset=%ld, length=%d\n", i, (long)lines[i].offset, lines[i].length);
    }
    printf("End of the table\n\n");
    printf("Total lines: %d\n\n", line_count);

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    while (1) {
        char input[256];
        char *endptr;
        
        printf("Enter line number (0 to exit): ");
        fflush(stdout);
        
        if (first_request) {
            alarm(5);
        }
        
        if (!fgets(input, sizeof(input), stdin)) {
            if (first_request) alarm(0);
            break;
        }
        
        if (first_request) {
            alarm(0);
        }
        
        first_request = 0;
        
        line_num = strtol(input, &endptr, 10);
        
        if (endptr == input || *endptr != '\n') {
            printf("Invalid input. Please enter a valid number.\n");
            continue;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > line_count) {
            printf("Invalid line number. Enter 1-%d or 0 to exit.\n", line_count);
            continue;
        }

        pos = lines[line_num - 1].offset;
        lseek(fd_global, pos, SEEK_SET);

        if (lines[line_num - 1].length > 0) {
            bytes_read = read(fd_global, buffer, lines[line_num - 1].length);
            if (bytes_read > 0) {
                write(STDOUT_FILENO, buffer, bytes_read);
                write(STDOUT_FILENO, "\n", 1);
            }
        }
    }

    close(fd_global);
    return 0;
}
