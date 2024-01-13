//
// Created by ascija on 1/13/24.
//

#include "unistd.h"
#include "stdio.h"

int main()
{
    size_t *address = (size_t*) 0x80089d6;
    void* p = sbrk(0);
    printf("%p \n",p);
    p = sbrk(10);
    printf("%p \n",p);
    p = sbrk(10);
    printf("%p \n",p);
    p = sbrk(10);
    printf("%p \n",p);

    *address = 15;

    printf("%ld \n",*address);

    return 0;
}