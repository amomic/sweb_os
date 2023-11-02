#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"


int main()
{

    printf("--------------------------------------\n"
           "Testing Fork\n");

    pthread_t* new_thread = 0;
    int return_value = pthread_create(new_thread, NULL, NULL, NULL);
    assert(return_value != 0);

    printf("Starting fork!");
    int retval = fork();

    printf("retval %d\n", retval);
    pthread_t* new_thread1 = 0;
    int return_value1 = pthread_create(new_thread1, NULL, NULL, NULL);
    assert(return_value1 != 0);

    printf("fork1  finished successful!\n");

    return 0;
}
