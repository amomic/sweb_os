#include "stdio.h"
#include "pthread.h"
#include "assert.h"
typedef struct parameterStruct
{
    int number;
}functionParameters;

void* startFunctionStructParam(void* param)
{
    functionParameters* output_params = (functionParameters*)param;
    printf("  [startFunctionWithStructParm] Number in parameter: %d\n", output_params->number);
    assert(output_params->number == 5);

    printf("  Successful!\n\n");
    return NULL;
}


int main()
{

    printf("--------------------------------------\n"
           "Testing Pthread Create 6 - struct params\n");

    pthread_t new_thread;

    functionParameters my_parameters;
    my_parameters.number = 5;

    int return_value = pthread_create(&new_thread, NULL,
                                      &startFunctionStructParam, (void*)&my_parameters);
    assert(return_value == 0);

    printf("pthread_create 6 finished successful!\n");

    return 0;
}
