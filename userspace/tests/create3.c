#include "stdio.h"
#include "pthread.h"
#include "assert.h"

void* startFunctionWithoutParams()
{
    printf("  In helper function!\n\n");
    return NULL;
}

int main()
{

    printf("--------------------------------------\n"
           "Testing Pthread Create - No params\n");

    pthread_t new_thread;
    int return_value = pthread_create(&new_thread, NULL,
                                      &startFunctionWithoutParams, NULL);
    assert(return_value == 0);

    printf("pthread_create 3 finished successful!\n");

    return 0;
}
