//2 threads
#include "pthread.h"
#include "stdio.h"
#include "assert.h"
#include "cancel_const.h"

pthread_t thread1;
pthread_t thread2;

void *func()
{
    pthread_setcanceltype(THREAD_CANCEL_TYPE_DEFERRED, NULL);
    pthread_setcancelstate(THREAD_CANCEL_STATE_ENABLED, NULL);
    printf("thread function\n");
    sleep(5);
    return 0;
}

int main()
{
    int ret1 = pthread_create(&thread1, NULL, &func, NULL);
    int ret2 = pthread_create(&thread2, NULL, &func, NULL);
    if (ret1 == 0 && ret2 == 0 )
    {
        printf("INFO: Threads successfully created\n");
    } else
    {
        printf("ERROR: Thread creation has failed\n");
    }

    int thread1_cancel = pthread_cancel(thread1);
    int thread2_cancel = pthread_cancel(thread2);

    if(thread1_cancel == 0 && thread2_cancel == 0)
    {
        printf("INFO: Threads successfully canceled\n");
    }
    else
    {
        printf("ERROR: Threads cancelation failed\n");
    }

    return 0;
}
