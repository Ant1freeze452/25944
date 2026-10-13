#include <stdio.h> // printf, fopen, fclose, perror
#include <unistd.h> // getuid, geteuid, setuid

static int check_file(void)
{
    FILE *file;

    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
    fflush(stdout); // выводим UID до возможного сообщения об ошибке

    file = fopen("data.txt", "r"); // файл с правами 600 в текущем каталоге
    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }

    printf("File opened successfully\n");
    if (fclose(file) == EOF)
    {
        perror("fclose");
        return 1;
    }

    return 0;
}

int main(void)
{
    int status;

    status = check_file();

    if (setuid(getuid()) == -1) // делаем эффективный UID равным реальному
    {
        perror("setuid");
        return 1;
    }

    if (check_file() != 0)
    {
        status = 1;
    }

    return status;
}
