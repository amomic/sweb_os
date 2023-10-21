//
// Created by mirza on 16.10.23.
// Multiple threads: creating and joining
//

#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* thread_function(void* arg) {
    int* value = (int*)arg;
    *value = 42; // Set a value to be returned
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
        int join_result = pthread_join(threads[i], &result);
        assert(join_result == 0);
        assert(values[i] == 42);
        printf("Thread %d result: %d;\nReturned: %d\n", i, (int)(intptr_t)result,join_result);
    }
    printf("   Test 2: Thread joined successful!\n\n");
    return 0;
}
