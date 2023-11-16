//
// Created by mirza on 16.11.23.
// Try to detach twice

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

    // Attempt to detach the thread again
    result = pthread_detach(thread);
    assert(result != 0 && "Attempting to detach a thread twice should fail");

    printf("Basic Detach Test 3 Passed - Try to detach twice\n");

    return 0;
}