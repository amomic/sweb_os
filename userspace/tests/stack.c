#define PAGE_SIZE 4096
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int routine = 1;
void threadFunction() {
    printf("start routine %d\n", routine);
    while(routine){

    }
}

int main()
{
    int j;
    pthread_t tid;
    pthread_create(&tid, NULL, (void* (*)(void*))threadFunction, NULL);
    sleep(5);

    char array[PAGE_SIZE * 5];
    for(int i = 0; i < PAGE_SIZE * 4; i++)
    {
        array[i] = (char)i;
        j = array[i];
    }

    printf("hello %d \n",j);

    return 0;
}

