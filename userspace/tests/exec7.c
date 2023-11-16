//
// Created by mirza on 16.11.23.
//

// exec_test.c
#include "stdio.h"
#include "pthread.h"
#include "unistd.h"
#include "stdlib.h"
#include "assert.h"

#define NUM_THREADS 5

void *threadFunction(void *arg) {
    int threadNumber = *((int *)arg);
    char *const argv[] = {"./usr/exechelper.sweb", NULL};

    printf("Thread %d is doing some work before exec...\n", threadNumber);

    int execResult = execv("./usr/exechelper.sweb", argv);

    assert(execResult != -1 && "execv failed");

    printf("Thread %d exec succeeded!\n", threadNumber);

    pthread_exit(NULL);
    return NULL;
}

int main() {
    int numThreads = NUM_THREADS;

    pthread_t threads[numThreads];

    for (int i = 0; i < numThreads; ++i) {
        int result = pthread_create(&threads[i], NULL, threadFunction, &i);
        assert(result == 0 && "Thread creation failed");
    }

    // Wait for all threads to finish
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}