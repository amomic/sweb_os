#include "stdio.h"
#include  "../libc/include/pthread.h"


int main(int argc, char *argv[])
{
    printf("\ntest execargsprint\n");

    printf("args count = %d\n", argc);

    for(int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    return 0;
}
