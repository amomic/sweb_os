//
// Created by mirza on 16.10.23.
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
    pthread_t thread;
    int res = pthread_create(&thread, NULL, thread_function, NULL);

    assert(!res && "FAIL");

    void *result;
    pthread_join(thread, &result);

    printf("Thread result: %d\n", (int)(intptr_t)result);

    return 0;
}