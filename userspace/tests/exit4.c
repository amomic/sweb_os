//
// Created by mirza on 16.11.23.
// Thread Exit and Join Order

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

void *threadFunction(void *arg) {
    int id = *((int *)arg);
    printf("Thread %d executing...\n", id);
    sleep(id);
    printf("Thread %d exiting...\n", id);
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t threads[3];
    int result;

    int threadIds[] = {0, 1, 2};

    for (int i = 0; i < 3; ++i) {
        result = pthread_create(&threads[i], NULL, threadFunction, &threadIds[i]);
        assert(result == 0 && "Thread creation failed");
    }

    for (int i = 0; i < 3; ++i) {
        result = pthread_join(threads[i], NULL);
        assert(result == 0 && "Thread join failed");
    }
    printf("Basic Thread Exit Test 4 Passed - Thread Exit and Join Order\n");

    return 0;
}
