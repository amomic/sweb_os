//
// Created by ascija on 12/13/23.
//
#include "stdio.h"
#include "unistd.h"

int main()
{
    //printf("%p\n", brk(2));
    // 134250496
    // 134250555
    int a = brk((void *) 0x00007D0000000000ULL);
    printf("%d\n", a);
    void *b = sbrk(2);
    printf("%p\n", b);
    void *c = sbrk(0);

    printf("%p\n", c);
    void *d = sbrk(2);

    printf("%p\n", d);

    return 0;
}
