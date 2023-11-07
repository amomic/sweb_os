//
// Created by mirza on 07.11.23.
// Boundary Test

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

int main() {
    char *command = "/usr/exechelper.sweb";
    char *args[] = {command, "This is a very long argument that exceeds the typical boundary by way to much and should not be allowed at all by any normal exec function in any possible way. LOREM IPSUM.This is a very long argument that exceeds the typical boundary by way to much and should not be allowed at all by any normal exec function in any possible way. LOREM IPSUM. This is a very long argument that exceeds the typical boundary by way to much and should not be allowed at all by any normal exec function in any possible way. LOREM IPSUM.", NULL};

    printf("Boundary Test - too long arg - should not execute\n");
    int result = execv(command, args);

    assert(result == -1 && "Exec should fail as expected");

    exit(0);
}