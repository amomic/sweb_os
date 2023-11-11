//
// Created by ascija on 06.11.23.
//
#include "unistd.h"
#include "stdio.h"

int main()
{
    unsigned int sleeping_time = 5;
    printf("Start\n");
    sleep(sleeping_time);
    printf("Mid\n");
    sleep(sleeping_time);
    printf("End\n");
    return 0;
}