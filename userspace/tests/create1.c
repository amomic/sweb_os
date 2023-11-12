#include "stdio.h"
#include "pthread.h"
#include "assert.h"

pthread_t thread[10];


void* thread_start(void *arg)
{

    printf("Hello world!\n");
    return NULL;
}


int main(int argc, char *argv[])
{

    printf("--------------------------------------\n"
           "Testing Pthread Create 1 - 5 threads in for loop\n");

    int number_of_threads = 5;
    pthread_t new_thread[number_of_threads];
    int return_value = -1;

    for(int cnt = 0; cnt < number_of_threads; cnt++)
    {
        return_value = pthread_create(&new_thread[cnt], NULL,
                                      &thread_start, NULL);
        assert(return_value == 0);
        printf("    pthread_create 1 for %d successful!\n", cnt);
    }
    sleep(3);

    /*for(int cnt = 0; cnt < 1000; cnt++)
    {
        printf("Hello\n");
    }*/

    return 0;
}
