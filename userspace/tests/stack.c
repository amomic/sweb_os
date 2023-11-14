//
// Created by mirza on 16.10.23.
// Basic join test
//
#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* thread_function(void* arg) {
    int value = 1;
    int* val = &value;
    while(1) {
        //val += 4096;
        val += 4096;
        *val = 2;
        printf("Value %d and Pointer %p\n", value, val);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    void* result = 0;

    int value = 0;
    pthread_create(&thread, NULL, thread_function, &value);



   while(1)
   {

   }

    assert(value == 42);
   // printf("Thread result: %d;\nReturned: %d\n", (int)(intptr_t)result,join_result);

    //printf("   Test 1: Thread joined successful!\n\n");
    return 0;
}
