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

    assert(retval == 0);

    printf("fork1  finished successful!\n");

    return 0;
}
