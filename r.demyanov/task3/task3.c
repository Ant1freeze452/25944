#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

static void try_open(const char *path)
{
    printf("real uid = %ld, effective uid = %ld\n",
           (long)getuid(), (long)geteuid());

    FILE *f = fopen(path, "r");

    if (f == NULL) {
        perror(path);
        return;
    }
    printf("%s открыт успешно\n", path);
    fclose(f);
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "data";

    try_open(path);

    if (setuid(getuid()) == -1) {       
        perror("setuid");
        return 1;
    }

    try_open(path);   
    return 0;
}