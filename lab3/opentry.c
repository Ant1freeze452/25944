#include <stdio.h>
#include <unistd.h>
void open_file(char *filename)
{
    printf("UID: %d, EUID: %d\n", getuid(), geteuid());

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening file");
    } else {
        printf("File opened\n");
        fclose(fp);
    }
}
int main(int argc, char **argv)
{
    char *filename;
    if (argc > 1) {
        filename = argv[1];
    } else {
        filename = "data.txt";
    }
    open_file(filename);
    setuid(getuid());
    open_file(filename);
    return 0;
}