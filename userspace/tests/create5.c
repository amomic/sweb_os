#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* startFunctionIntParam(void* param)
{
    assert(*(int*)param == 5);
    return NULL;
}


int main()
{

    printf("--------------------------------------\n"
           "Testing Pthread Create 5 - int params\n");

    pthread_t new_thread;
    int param = 5;
    int return_value = pthread_create(&new_thread, NULL,
                                      &startFunctionIntParam, (void*)(size_t)param);

    assert(return_value == 0);

    printf("pthread_create 5 finished successful!\n");

    return 0;
}
