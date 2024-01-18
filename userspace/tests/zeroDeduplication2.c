//
// Created by amar on 1/18/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wait.h>
#include <unistd.h>
#include <sched.h>
#define SIZE (4096)

int main() {
    //sleep(2);

    int *arr = malloc(SIZE* sizeof(int)* sizeof(int)* sizeof(int));

    for (int i = 0; i < SIZE; ++i)
    {
        if(i == SIZE/2)
        {
            arr[i] =5;
        }

        sched_yield();

        printf("%d", arr[i]);
    }
    //sleep(2);
}