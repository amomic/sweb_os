//
// Created by amar on 11/30/23.
//
#include "stdio.h"
#include "pthread.h"
#include "assert.h"


void* func0(void *arg)
{

    printf("0!\n");
    return NULL;
}

void* func1(void *arg)
{

    printf("1!\n");
    return NULL;
}
void* func2(void *arg)
{

    printf("2!\n");
    return NULL;
}

void* func3(void *arg)
{

    printf("3!\n");
    return NULL;
}

// Array of function pointers
typedef void* (*func_ptr)(void *);
func_ptr funcs[4] = {func0, func1, func2, func3};


int main(int argc, char *argv[])
{

    printf("--------------------------------------\n"
           "Testing pthread multiple\n");

    pthread_t thread;
    pthread_multiple((pointer **) &thread, NULL, (pointer **) (void *(*)(void *)) &funcs, 4, NULL);

    pthread_exit(0);
}
