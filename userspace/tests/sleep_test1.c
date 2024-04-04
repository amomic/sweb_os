//
// Created by ascija on 06.11.23.
//
#include "unistd.h"
#include "time.h"
#include "stdio.h"
int main()
{
    unsigned int sleeping_time = 5;
    clock_t  t1 = clock();
    sleep(sleeping_time);
    clock_t  t2 = clock();
    printf("%f", ((double) t2 - (double) t1) / CLOCKS_PER_SEC);
    return 0;
}