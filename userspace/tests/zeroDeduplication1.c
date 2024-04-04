//
// Created by amar on 1/15/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#define SIZE 4096

int main ()
{
    //sleep(2);

    int array[SIZE];

    //sleep(2);
    printf("Yielding!\n");
    sched_yield();

    for(int i = 0; i < SIZE; ++i)
    {
        if(i == SIZE/2)
        {
            array[i] = 5;
        }

        printf("%d", array[i]);
    }
    //sleep(2);
}

