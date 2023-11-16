//
// Created by mirza on 16.11.23.
// Basic Thread Exit

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int globalVar = 0;

void *threadFunction(void *arg) {
    globalVar++;
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, threadFunction, NULL);
    assert(result == 0 && "Thread creation failed");

    // Wait for the thread to finish
    result = pthread_join(thread, NULL);
    assert(result == 0 && "Thread join failed");

    // Verify that the variable is updated by the thread
    assert(globalVar == 1 && "Global variable not updated by the thread");

    printf("Basic Thread Exit Test 1 Passed - Basic Thread Exit\n");

    return 0;
}
