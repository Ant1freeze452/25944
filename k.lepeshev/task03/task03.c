#include <stdio.h>
#include <unistd.h> // для getuid()
#include <stdlib.h>


int main(){

    printf("First step before setuid\n");
    printf("Real UID: %lu\n", (unsigned long)getuid());
    printf("Effective UID: %lu\n", (unsigned long)geteuid());

    FILE *file_in = fopen("vip_file.txt", "r");

    if(file_in == NULL){
        perror("fopen");
    }
    else{
        printf("File opened successfully\n");
        fclose(file_in);
    }
    printf("setuid(getuid()) attempt\n");
    if(setuid(getuid()) == -1){
        perror("setuid");
    }
    else{
        printf("setuid(getuid()) success\n");
    }

    printf("Second step after setuid\n");
    printf("Real UID: %lu\n", (unsigned long)getuid());
    printf("Effective UID: %lu\n", (unsigned long)geteuid());

    file_in = fopen("vip_file.txt", "r");

    if(file_in == NULL){
        perror("fopen");
    }
    else{
        printf("File opened successfully\n");
        fclose(file_in);
    }


    exit(0);
}