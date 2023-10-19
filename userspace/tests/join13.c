//
// Created by mirza on 19.10.23.
// Joining a thread that uses condition variables
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

pthread_cond_t condition;
pthread_mutex_t mutex;
int shared_data = 0;
int thread_completed = 0;

void* thread_function(void* arg) {
    pthread_mutex_lock(&mutex);
    shared_data = 42;

    thread_completed = 1;
    pthread_cond_signal(&condition);

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t thread;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);

    pthread_create(&thread, NULL, thread_function, NULL);

    pthread_mutex_lock(&mutex);
    while (!thread_completed) {
        pthread_cond_wait(&condition, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    int join_result = pthread_join(thread, NULL);
    printf("Thread with cond returned: %d\n",join_result);

    assert(join_result == 0);
    assert(shared_data == 42);

    printf("  Test 13: Joining a thread that uses condition variables handled successfully!\n\n");

    return 0;
}
