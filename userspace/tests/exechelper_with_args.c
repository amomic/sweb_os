//
// Created by mirza on 03.11.23.
//
#include "stdio.h"

int main(int argc, char **argv) {
    printf("Received arguments:\n");

    for (int i = 0; i < argc; i++) {
        printf("Arg %d : ", i);
        printf("%s\n", argv[i]);
    }

    return 0;
}