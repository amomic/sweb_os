//
// Created by mirza on 19.10.23.
// Interrupting waiting threads
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    sleep(5);
    return NULL;
}

int main() {
    pthread_t thread;

    pthread_create(&thread, NULL, thread_function, NULL);
    printf("Thread created!\n");

    int interrupt_result = pthread_cancel(thread);
    int join_result = pthread_join(thread, NULL);

    assert(interrupt_result == 0);
    printf("Thread canceled with return val: %d\n", interrupt_result);
    assert(join_result == 0);
    printf("Thread Returned: %d\n", join_result);

    printf("   Test 12: Interrupting waiting threads handled successfully!\n\n");

    return 0;
}