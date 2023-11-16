//
// Created by mirza on 16.11.23.
// Thread Exit and Returning Values

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

void *threadFunction(void *arg) {
    int returnValue = 42;

    pthread_exit((void *)(intptr_t)returnValue);
    return NULL;
}

int main() {
    pthread_t thread;
    int result;
    void *returnValue;

    result = pthread_create(&thread, NULL, threadFunction, NULL);
    assert(result == 0 && "Thread creation failed");

    // Wait for the thread to finish and retrieve the return value
    result = pthread_join(thread, &returnValue);
    assert(result == 0 && "Thread join failed");

    assert((int)(intptr_t)returnValue == 42 && "Returned value does not match");

    printf("Basic Thread Exit Test 2 Passed - Thread Exit and Returning Values\n");

    return 0;
}