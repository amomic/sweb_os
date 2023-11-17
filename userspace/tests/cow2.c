#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"

void* func(){

    int data = 0;
    pid_t pid = fork();
    if (pid == -1) {
        // Error handling for fork failure
        printf("Fork failed");
        exit(-1);
    }

    if (pid == 0) {
        printf("Child process - Before changing data: %d\n", data);

        // Change the data in the child process
        data = 1;
        printf("Child process - After changing data: %d\n", data);

    } else {

        printf("Parent process - Before %d\n", data);
        data = 2;
        printf("Parent process - after %d\n", data);

    }
    pthread_exit(0);
    return NULL;
}

int main()
{
    pthread_t tid;

    pthread_create(&tid, NULL,  func, NULL);
    getchar();
    return 0;
}
