// one thread

#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "cancel_const.h"

pthread_t thread1;

void func()
{
    pthread_setcanceltype(THREAD_CANCEL_TYPE_DEFERRED, NULL);
    pthread_setcancelstate(THREAD_CANCEL_STATE_DISABLED, NULL);
    sleep(10);
}

int main()
{
    int ret = pthread_create(&thread1, NULL, (void *) &func, NULL);
    if (ret == 0)
    {
        printf("INFO: Threads successfully created\n");
    } else
    {
        printf("ERROR: Thread creation has failed\n");
    }

    int thread1_cancel = pthread_cancel(thread1);

    if (thread1_cancel == 0 )
    {
        printf("INFO: Thread 1 successfully canceled\n");
    }
    else
    {
        printf("ERROR: Thread 1 cancelation failed\n");
    }

    return 0;
}
