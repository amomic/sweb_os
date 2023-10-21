//
// Created by mirza on 19.10.23.
// Edge Case: Valid and invalid thread IDs
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
    pthread_t valid_thread;
    pthread_t invalid_thread = (pthread_t)12345; // Invalid thread ID

    pthread_create(&valid_thread, NULL, thread_function, NULL);

    void* result1;
    void* result2;
    int join_result1 = pthread_join(valid_thread, &result1);
    int join_result2 = pthread_join(invalid_thread, &result2); // Attempt joining an invalid thread
    assert(join_result1 == 0);
    printf("Thread result: %d;\nReturned: %d\n", (int)(intptr_t)result1,join_result1);
    assert(join_result2 != 0);
    printf("* Joining invalid thread result: %d;\nReturned: %d\n", (int)(intptr_t)result2,join_result2);

    printf("   Test 9: Edge Case: Valid and invalid thread IDs handled successfully!\n\n");

    return 0;
}