#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* thread_function(void* arg) {
    int value = 1;
    int* val = &value;
    while(1) {
        val += 4096;
        *val = 2;
        printf("Value %d and Pointer %p\n", value, val);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int value = 0;
    pthread_create(&thread, NULL, thread_function, &value);



    while(1)
    {
        printf("hi");
    }

    return 0;
}
