#include "stdio.h"
#include "assert.h"
#define SIZE 1024ULL * 1024ULL
int array[SIZE] = {0};

int main(int argc, char *argv[])
{
    printf("test started");
    int a = 0;

    for (int i = 0; i < SIZE; i+=1024)
    {
        a = array[i];
    }
    for (int i = 0; i < SIZE; i+=1024)
    {
        a = array[i];
    }

    for (int i = 0; i < SIZE; i+=1024)
    {
        printf("%d ", array[i]);
    }
    for (int i = 0; i < SIZE; i+=1024)
    {
        printf("%d ", array[i]);
    }

    printf("test finished with a= %d\n",a);

    return 0;
}