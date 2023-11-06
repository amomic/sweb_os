// multiple with loop
#include "stdio.h"
#include "pthread.h"
#include "cancel_const.h"
#include "assert.h"

#define THREAD_COUNT 10
#define TIMER 1024*1024*1024

size_t tid[THREAD_COUNT];

void loop()
{
    pthread_setcanceltype(THREAD_CANCEL_TYPE_DEFERRED, NULL);
    pthread_setcancelstate(THREAD_CANCEL_STATE_DISABLED, NULL);
    for (int i = 0; i < TIMER; i++)
    {}
}

int main()
{
    for (int t = 0; t < THREAD_COUNT; t++)
    {
        pthread_create(&tid[t], NULL, (void *) &loop, (void *) 1);
    }
    for (int j = 0; j < THREAD_COUNT; j++)
    {
        int res = pthread_cancel(tid[j]);
        assert(res == 0 && "ERROR: Thread 1 cancelation failed\n");
        if (res == 0)
        {
            printf("INFO: Thread %zu successfully canceled\n", tid[j]);
        } else
        {
            printf("ERROR: Thread %zu cancelation failed\n", tid[j]);
        }
    }
    return 0;
}