#include "stdio.h"
#include "pthread.h"
#include "assert.h"

int main()
{

    printf("--------------------------------------\n"
           "Testing Pthread Create 2 - No start routine\n");

    pthread_t* new_thread = 0;
    int return_value = pthread_create(new_thread, NULL, NULL, NULL);
    assert(return_value != 0);

    printf("pthread_create 2 finished successful!\n");

    return 0;
}
