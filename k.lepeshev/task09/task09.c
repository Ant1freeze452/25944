#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


int main(void){

    printf("Parent process: creating child\n");
    fflush(stdout);

    pid_t pid = fork();

    if(pid == -1){
        perror("fork");
        exit(1);
    }
    else if(pid == 0){
        execlp("cat", "cat", "text.txt", (char *)NULL);

        perror("execlp");
        _exit(1);
    }
    else{
        if(waitpid(pid, NULL, 0) == -1){
            perror("waitpid");
            exit(1);
        }
        printf("Parent process: child has finished\n");
    }

    exit(0);
}