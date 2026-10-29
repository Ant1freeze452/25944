#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_LINES 1000

typedef struct {
    off_t offset;
    int length;
} LineInfo;

static int fd_global;
static size_t file_size_global;
static char *mapped_global;

void alarm_handler(int sig) {
    (void)sig;
    
    printf("\nTime's over, printing entire file:\n");
    if (mapped_global) {
        write(STDOUT_FILENO, mapped_global, file_size_global);
    }
    
    if (mapped_global) {
        munmap(mapped_global, file_size_global);
    }
    close(fd_global);
    exit(0);
}

int main(int argc, char *argv[]) {
    const char *filename;
    LineInfo lines[MAX_LINES];
    int line_count = 1;
    int i;
    int line_num;
    off_t pos;
    struct sigaction sa;
    struct stat st;

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

    if (fstat(fd_global, &st) < 0) {
        perror("Error getting file size");
        close(fd_global);
        return 1;
    }

    file_size_global = st.st_size;

    if (file_size_global == 0) {
        printf("Empty file.\n");
        close(fd_global);
        return 0;
    }

    mapped_global = mmap(NULL, file_size_global, PROT_READ, MAP_PRIVATE, fd_global, 0);
    if (mapped_global == MAP_FAILED) {
        perror("Error mapping file");
        close(fd_global);
        return 1;
    }

    lines[0].offset = 0;
    lines[0].length = 0;
    line_count = 1;

    for (i = 0; i < (int)file_size_global && line_count < MAX_LINES; i++) {
        if (mapped_global[i] == '\n') {
            lines[line_count - 1].length = i - lines[line_count - 1].offset;
            lines[line_count].offset = i + 1;
            lines[line_count].length = 0;
            line_count++;
        }
    }

    if (lines[line_count - 1].offset < file_size_global) {
        lines[line_count - 1].length = file_size_global - lines[line_count - 1].offset;
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
        
        alarm(5);
        
        if (!fgets(input, sizeof(input), stdin)) {
            alarm(0);
            break;
        }
        
        alarm(0);
        
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

        if (lines[line_num - 1].length > 0) {
            write(STDOUT_FILENO, mapped_global + pos, lines[line_num - 1].length);
            write(STDOUT_FILENO, "\n", 1);
        }
    }

    munmap(mapped_global, file_size_global);
    close(fd_global);
    return 0;
}
