//
// Created by ascija on 06.11.23.
//

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"

void function1()
{
    unsigned int sleep_return_value = sleep(5);
    printf("Thread 1 done sleeping. \n %u", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

void function2()
{
    unsigned int sleep_return_value = sleep(10);
    printf("Thread 2 done sleeping. \n %u", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

void function3()
{
    unsigned int sleep_return_value = sleep(NULL);
    printf("Thread 3 done sleeping. \n %u", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

int main()
{
    pthread_t thread1, thread2, thread3;
    int success = pthread_create(&thread1, NULL, (void *(*)(void *)) &function1, NULL);
    assert(success == 0 && "Success 0. \n");
    printf("Created:1");
    success = pthread_create(&thread2, NULL, (void *(*)(void *)) &function2, NULL);
    assert(success == 0 && "Success 0. \n");
    printf("Created:2");
    success = pthread_create(&thread3, NULL, (void *(*)(void *)) &function3, NULL);
    assert(success == 0 && "Success 0. \n");
    printf("Created:3");
    unsigned int sleep_return_value = sleep(10);
    printf("Thread 0 done sleeping. \n %u", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");

    return 0;
}