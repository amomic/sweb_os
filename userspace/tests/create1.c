#include "stdio.h"
#include "pthread.h"
#include "assert.h"

pthread_t thread[10];


static void * thread_start(void *arg)
{
    printf("%s - ", (char *) arg);
    printf("I'm here\n");
    return (void*)-1;
}


int main(int argc, char *argv[])
{

    //pthread_t something;
    int return_value;


    for(int i = 0; i < 200; i++)
    {
        return_value = pthread_create(&thread[i], NULL, &thread_start, "Hi!");
        assert(!return_value);
    }

    return 0;
}