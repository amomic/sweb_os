//
// Created by mirza on 07.11.23.
// Base test

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

int main() {
    printf("This is the original program.\n");

    char* programPath = "./usr/exechelper.sweb";

    char* const argv[] = {programPath, NULL};

    execv(programPath, argv);


    assert(0); // This line should not be reached if execv is successful

    return 0;
}