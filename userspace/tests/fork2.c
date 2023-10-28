#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"


int main()
{
    printf("before fork!\n");
    pthread_t thread;
    pthread_create(&thread, NULL, NULL, NULL);

    printf("\n");
    size_t pid = fork();
    printf("fork1 is %zu\n", pid);
    for (int i = 0; i < 10000; ++i) {

    }

    for (int i = 0; i < 10000; ++i) {

    }

    printf("\n");
    size_t pid1 = fork();
    printf("fork2 is %zu\n", pid1);
    for (int i = 0; i < 10000; ++i) {
    }

    for (int i = 0; i < 10000; ++i) {

    }
    printf("\n");
    size_t pid2 = fork();
    printf("fork3 is %zu\n", pid2);

    return 0;
}

