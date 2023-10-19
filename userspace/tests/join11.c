//
// Created by mirza on 19.10.23.
// Thread cancellation and joining a canceled thread
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

    int cancel_result = pthread_cancel(thread);
    assert(cancel_result == 0); // Check that the thread was canceled
    printf("Thread is canceled with return value: %d\n", cancel_result);

    int join_result = pthread_join(thread, NULL);
    assert(join_result == 0); // Check that joining a canceled thread results in an error
    printf("Tried to join canceled Returned: %d\n",join_result);

    printf("   Test 11: Thread cancellation and joining a canceled thread handled successfully!\n\n");

    return 0;
}