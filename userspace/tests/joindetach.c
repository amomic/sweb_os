//
// Created by mirza on 19.10.23.
// Attempted joining a detached thread
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"


void* thread_function(void* arg) {
    int count;
    for (count = 0; count < 10; count++);
    assert(count==10);
    return NULL;
}

int main() {
    pthread_t thread;

    pthread_create(&thread, NULL, thread_function, NULL);
    printf("Thread created!\n");

    int detach_result = pthread_detach(thread);
    assert(detach_result == 0);
    printf("Thread detached, detach return: %d\n", detach_result);

    void* result;
    int join_result = pthread_join(thread, &result);
    assert(join_result != 0);
    printf("Thread result: %d;\nReturned: %d\n", (int)(intptr_t)result,join_result);

    printf("   Test join & detach: Edge Case: Attempted joining a detached thread handled successfully!\n\n");

    return 0;
}