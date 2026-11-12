#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        execlp("cat", "cat", argv[1], (char *)0);

        perror("execlp");
        _exit(1);
    }

    printf("Parent process is running\n");
    fflush(stdout);

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return 1;
    }

    printf("Child process has finished\n");

    return 0;
}