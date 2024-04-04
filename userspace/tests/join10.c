//
// Created by mirza on 19.10.23.
// Multiple joins / including join on the same thread again
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
    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, thread_function, NULL);
    pthread_create(&thread2, NULL, thread_function, NULL);

    void* result1, *result2, *result3;
    int join_result1 = pthread_join(thread1, &result1);
    int join_result2 = pthread_join(thread2, &result2);
    int join_result3 = pthread_join(thread1, &result3); // Join the same thread again

    assert(join_result1 == 0);
    printf("Thread 1 result: %d;\nReturned: %d\n", (int)(intptr_t)result1 ,join_result1);
    assert(join_result2 == 0);
    printf("Thread 2 result: %d;\nReturned: %d\n", (int)(intptr_t)result2 ,join_result2);
    assert(join_result3 != 0);
    printf("Thread 1 joining again result: %d;\nReturned: %d\n", (int)(intptr_t)result3 ,join_result3);

    printf("   Test 10: Multiple joins handled successfully!\n\n");

    return 0;
}
