//
// Created by mirza on 16.10.23.
// Concurrent join
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    int* value = (int*)arg;
    *value = 42;
    return NULL;
}

int main() {
    pthread_t threads[5];
    int values[5] = {0};

    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, thread_function, &values[i]);
    }

    void* result;

    for (int i = 0; i < 5; i++) {
        pthread_t thread = threads[i];
        int join_result = pthread_join(thread, &result);
        assert(join_result == 0);
        assert(values[i] == 42);
        printf("Thread %d result: %d;\nReturned: %d\n", i, (int)(intptr_t)result, join_result);
    }

    printf("   Test 4: All threads joined concurrently with return values of 42!\n\n");
    return 0;
}