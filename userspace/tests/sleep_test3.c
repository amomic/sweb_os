//
// Created by ascija on 06.11.23.
//
#include "unistd.h"
#include "stdio.h"
int main()
{
    unsigned int sleeping_time = 5;
    printf("Start");
    sleep(sleeping_time);
    printf("Mid");
    sleep(sleeping_time);
    printf("End");
    return 0;
}