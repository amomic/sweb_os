#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* startFunctionSizetParam(void* param)
{
    printf("Number in param: %lu\n", (size_t)param);
    assert((size_t)param == 5);
    printf("  Successful!\n\n");
    return NULL;
}


int main()
{

    printf("--------------------------------------\n"
           "Testing Pthread Create - Size_t params\n");

    pthread_t new_thread;
    size_t param = 5;
    int return_value = pthread_create(&new_thread, NULL,
                                      &startFunctionSizetParam, (void*)param);
    assert(return_value == 0);

    printf("pthread_create 3 finished successful!\n");

    return 0;
}
