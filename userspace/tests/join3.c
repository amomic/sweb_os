//
// Created by mirza on 16.10.23.
// Threads attempting to join themselves indirectly
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"


void* join_thread_function(void* arg) {
    pthread_t* target_thread = (pthread_t*)arg;
    int join_result = pthread_join(*target_thread, NULL);
    assert(join_result != 0);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, join_thread_function, &thread1);
    pthread_create(&thread2, NULL, join_thread_function, &thread2);

    int join_result1 = pthread_join(thread1, NULL);
    int join_result2 = pthread_join(thread2, NULL);

    assert(join_result1 == 0);
    assert(join_result2 == 0);

    printf("   Test 3: Edge Case: Threads attempting to join themselves indirectly handled successfully!\n");
    return 0;
}
