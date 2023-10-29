// multiple with loop 2
#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "cancel_const.h"

#define THREAD_COUNT 10

size_t tid[THREAD_COUNT];

void loop()
{
    pthread_setcanceltype(THREAD_CANCEL_TYPE_ASYNC, NULL);
    pthread_setcancelstate(THREAD_CANCEL_STATE_ENABLED, NULL);
    printf("INFO: Threads successfully created\n");
    sleep(4);
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
        assert(res == 0 && "ERROR: Thread cancelation failed\n");
    }
    return 0;
}