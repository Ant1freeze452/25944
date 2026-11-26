#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    pid_t pid;

    if (pipe(fd) == -1)
    {
        perror("pipe");
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
        char buffer[100];
        ssize_t count;

        close(fd[1]);

        while ((count = read(fd[0], buffer, sizeof(buffer))) > 0)
        {
            for (int i = 0; i < count; i++)
            {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }

            write(1, buffer, count);
        }

        close(fd[0]);
        _exit(0);
    }

    close(fd[0]);

    char text[] = "Hello my DeAr FriEnd!\n";

    write(fd[1], text, sizeof(text) - 1);
    close(fd[1]);

    waitpid(pid, NULL, 0);

    return 0;
}