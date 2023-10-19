//
// Created by mirza on 19.10.23.
// Testing thread return values
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    int* value = (int*)arg;
    *value = 42; // Set a value to be returned
    return value;
}

int main() {
    pthread_t thread;
    int result = 0;

    int value = 0;
    pthread_create(&thread, NULL, thread_function, &value);

    void* thread_return;
    int join_result = pthread_join(thread, &thread_return);
    assert(join_result == 0);
    assert(value == 42);
    assert(*(int*)thread_return == 42);
    printf("Thread result: %d;\nReturned: %d\n", (int)(intptr_t)thread_return ,join_result);

    printf("  Test8: Thread return values handled successfully!\n\n");

    return 0;
}
