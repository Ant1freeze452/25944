#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_id()
{
    printf("Real UID: %d, Effective UID: %d\n", (int)getuid(), (int)geteuid());
    FILE *f = fopen("3.txt", "r");
    if (f == NULL)
    {
        perror("fopen error");
    }
    else
    {
        printf("file opened successfully\n");
        fclose(f);
    }
}

int main()
{
    print_id();

    if (setuid(getuid()) == -1)
    {
        perror("setuid failed");
    }

    print_id();
    return 0;
}