#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s command [arguments...]\n", argv[0]);
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
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(123);
    }

    while (waitpid(pid, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            perror("waitpid");
            return 1;
        }
    }

    if (WIFEXITED(status))
    {
        printf("Exit code: %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("Terminated by signal: %d\n", WTERMSIG(status));
    }

    return 0;
}