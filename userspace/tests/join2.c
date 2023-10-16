//
// Created by mirza on 16.10.23.
// Multiple threads: creating and joining
//

#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void *thread_function(void *arg) {
    int i = 0;
    for(i = 0; i < 20; i++);
    assert(i=20);
    return NULL;
}

int main() {
    pthread_t threads[5];

    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, thread_function, NULL);
    }

    void *result;
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], &result);
        printf("Thread %d result: %d\n", i, (int)(intptr_t)result);
    }

    return 0;
}
