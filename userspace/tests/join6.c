//
// Created by mirza on 19.10.23.
// Performance test for pthread_join
//
#include "stdio.h"
#include "assert.h"
#include "pthread.h"

#define NUM_THREADS 15

void* thread_function(void* arg) {
    int count;
    for (count = 0; count < 10; count++);
    assert(count==10);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_function, NULL);
        printf("Thread %d created!\n", i);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        int join_result = pthread_join(threads[i], NULL);
        assert(join_result == 0);
        printf("Thread %d joined! With return: %d\n", i, join_result);
    }

    printf("   Test 6: All threads joined successfully!\n\n");
    return 0;
}