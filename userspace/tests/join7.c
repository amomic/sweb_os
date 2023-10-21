//
// Created by mirza on 19.10.23.
// Attempt joining a non-existent thread
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
    void* result;
    pthread_t invalid_thread = (pthread_t)12345;
    int join_result = pthread_join(invalid_thread, &result);
    assert(join_result != 0);
    printf("Thread result: %d;\nReturned: %d\n", (int)(intptr_t)result,join_result);

    printf("   Test 7: Edge Case: Attempted joining a non-existent thread handled successfully!\n\n");
    return 0;
}