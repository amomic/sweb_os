//
// Created by mirza on 16.11.23.
// Detachment of Already Running Threads

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void *threadFunction(void *arg) {
    sleep(2);
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, threadFunction, NULL);
    assert(result == 0 && "Thread creation failed");

    sleep(1);

    result = pthread_detach(thread);

    // Check the result and print it
    printf("DETACHED: %d\n", result);

    result = pthread_join(thread, NULL);
    assert(result == -1 && "Joined the thread");

    printf("Basic Detach Test 7 Passed\n");

    return 0;
}