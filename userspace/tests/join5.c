//
// Created by mirza on 19.10.23.
// Recursive join
//
#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    return NULL;
}

int main() {
    pthread_t thread;

    void* result1, *result2;

    pthread_create(&thread, NULL, thread_function, NULL);

    int join_result1 = pthread_join(thread, &result1);
    int join_result2 = pthread_join(thread, &result2); // Try joining the same thread again

    assert(join_result1 == 0);
    printf("Thread 1 result: %d;\nReturned: %d\n", (int)(intptr_t)result1,join_result1);
    assert(join_result2 != 0);
    printf("Thread 2 result: %d;\nReturned: %d\n", (int)(intptr_t)result2,join_result2);


    printf("   Test 5: Recursive join handled successfully!\n\n");
    return 0;
}