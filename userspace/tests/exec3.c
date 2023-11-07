//
// Created by mirza on 07.11.23.
// Confirming Null Arguments

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

int main() {
    printf("Confirming Null Arguments\n");
    char *command = "/usr/exechelper.sweb";
    char *args[] = {command, NULL};

    int result = execv(command, args);

    assert(result == 0 && "Exec should succeed when provided with a valid program path and NULL argument");

    exit(0);
}