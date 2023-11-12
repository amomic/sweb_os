#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wait.h>

int main()
{
    int status = 0;
    size_t pid = fork();
    if(pid == 0)
    {
        printf("hi from child\n");
        //sleep(5);
        exit(-10);
    }
    else
    {
        printf("helooo from parent\n");
        sleep(5);
        int retval = waitpid(pid, &status,0);
        printf("%d",status);
        printf("ret is %d\n", retval);
    }
    return 0;
}

