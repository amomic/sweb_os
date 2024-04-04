//
// Created by mirza on 16.11.23.
// Detach a Thread and Attempt to Join It

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

void *threadFunction(void *arg) {
    printf("Thread executing...\n");
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, threadFunction, NULL);
    assert(result == 0 && "Thread creation failed");

    result = pthread_detach(thread);
    assert(result == 0 && "Thread detachment failed");

    // Attempt to join the detached thread
    result = pthread_join(thread, NULL);
    assert(result != 0 && "Attempting to join a detached thread should fail");

    printf("Basic Detach Test 2 Passed - Detach a Thread and Attempt to Join It\n");

    return 0;
}
