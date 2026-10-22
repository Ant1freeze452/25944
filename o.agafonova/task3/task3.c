
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void check_file(const char *filename)
{
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());

    FILE *file = fopen(filename, "r");

    if (file != NULL) {
        printf("Файл %s успешно открыт\n", filename);
        fclose(file);
    } else {
        perror("Ошибка открытия файла");
    }
}

int main(int argc, char *argv[])
{
    const char *filename = "data";

    if (argc > 1) {
        filename = argv[1];
    }

    printf("До вызова setuid():\n");
    check_file(filename);

    if (setuid(getuid()) != 0) {
        perror("Ошибка setuid");
        return 1;
    }

    printf("\nПосле вызова setuid():\n");
    check_file(filename);

    return 0;
}
