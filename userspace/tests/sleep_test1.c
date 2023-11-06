//
// Created by ascija on 06.11.23.
//
#include "unistd.h"
#include "time.h"
#include "stdio.h"

int main()
{
    unsigned int sleeping_time = 5;
    clock_t start = clock();
    printf("Start time: %u\n", start);
    sleep(sleeping_time);
    clock_t after = clock();
    printf("Time after loop: %u\n", after);
    double total = (double) (after - start) / CLOCKS_PER_SEC;
    printf("Total time taken by CPU: %f\n", total);
    return 0;
}