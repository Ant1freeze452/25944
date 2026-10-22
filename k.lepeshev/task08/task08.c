#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main(void){

    printf("Parent process: creation child\n");
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
        printf("Parent process: last message\n");
    }



    exit(0);
}