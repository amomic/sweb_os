//
// Created by mirza on 19.10.23.
// Basic Detach Test

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    int* value = (int*)arg;
    *value = 42;
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, thread_function, NULL);
    assert(result == 0 && "Thread creation failed");

    // Detach the thread
    result = pthread_detach(thread);
    assert(result == 0 && "Thread detachment failed");

    // Check the result and print it
    printf("DETACHED: %d\n", result);

    printf("Detach Test 1 Passed - Basic Detach Test\n");

    return 0;
}