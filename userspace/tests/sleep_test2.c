//
// Created by ascija on 06.11.23.
//

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "time.h"

void function1()
{
    clock_t  t1 = clock();
    unsigned int sleep_return_value = sleep(5);
    clock_t  t2 = clock();
    printf("%f\n", ((double) t2 - (double) t1) / CLOCKS_PER_SEC);
    printf("Thread 1 done sleeping. %u\n", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

void function2()
{
    clock_t  t1 = clock();
    unsigned int sleep_return_value = sleep(10);
    clock_t  t2 = clock();
    printf("%f\n", ((double) t2 - (double) t1) / CLOCKS_PER_SEC);
    printf("Thread 2 done sleeping.  %u\n", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

void function3()
{
    clock_t  t1 = clock();
    unsigned int sleep_return_value = sleep(NULL);
    clock_t  t2 = clock();
    printf("%f\n", ((double) t2 - (double) t1) / CLOCKS_PER_SEC);
    printf("Thread 3 done sleeping. %u\n", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");
}

int main()
{
    pthread_t thread1, thread2, thread3;
    int success = pthread_create(&thread1, NULL, (void *(*)(void *)) &function1, NULL);
    assert(success == 0 && "Success 0. \n");
    success = pthread_create(&thread2, NULL, (void *(*)(void *)) &function2, NULL);
    assert(success == 0 && "Success 0. \n");
    success = pthread_create(&thread3, NULL, (void *(*)(void *)) &function3, NULL);
    assert(success == 0 && "Success 0. \n");
    clock_t  t1 = clock();
    unsigned int sleep_return_value = sleep(10);
    clock_t  t2 = clock();
    printf("%f\n", ((double) t2 - (double) t1) / CLOCKS_PER_SEC);
    printf("Thread 0 done sleeping. %u\n", sleep_return_value);
    assert(sleep_return_value == 0 && "sleep() return 0 \n");

    return 0;
}