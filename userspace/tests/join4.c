//
// Created by mirza on 16.10.23.
// Joining on completed thread
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void *thread_function(void *arg) {
    int i = 0;
    for(i = 0; i < 20; i++);
    assert(i=20);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_function, NULL);

    // Simulate work here that may take some time

    void *result;
    pthread_join(thread, &result);
    printf("Thread result: %d\n", (int)(intptr_t)result);

    return 0;
}