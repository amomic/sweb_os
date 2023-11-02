
#include "stdio.h"
#include  "../libc/include/unistd.h"
#include "pthread.h"

#define NR_THREADS 21

void start_routine()
{
    size_t pid = fork();

    if(pid != 0)
    {
        printf("hello from the parent process. My id is %zu\n", pid);
    }
    else
    {
        printf("hello from the child process. My id is %zu\n", pid);
        return;
    }

}

int main()
{
    size_t tid[NR_THREADS];

        for(int i = 0; i < NR_THREADS; i++)
        {
            pthread_create(&tid[i], NULL, (void*)&start_routine, (void*)7);
        }



    // wait
    for (int i = 0; i < 10000; ++i) {
        for (int j = 0; j < 5000; ++j) {

        }
    }

    return 0;
}
