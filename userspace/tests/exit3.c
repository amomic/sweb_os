//
// Created by mirza on 16.11.23.
// Multiple Threads Exiting

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

void *threadFunction(void *arg) {
    int id = *((int *)arg);
    printf("Thread %d executing...\n", id);
    sleep(2);
    printf("Thread %d exiting...\n", id);
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t threads[3];
    int result;

    // Create multiple threads
    int threadIds[] = {0, 1, 2};

    for (int i = 0; i < 3; ++i) {
        result = pthread_create(&threads[i], NULL, threadFunction, &threadIds[i]);
        assert(result == 0 && "Thread creation failed");
    }

    // Wait for the threads to finish
    for (int i = 0; i < 3; ++i) {
        result = pthread_join(threads[i], NULL);
        assert(result == 0 && "Thread join failed");
    }

    printf("Basic Thread Exit Test 3 Passed - Multiple Threads Exiting\n");

    return 0;
}