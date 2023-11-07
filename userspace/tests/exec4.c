//
// Created by mirza on 03.11.23.
// Invalid Command Test

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

int main() {
    printf("Invalid command test - should not execute");
    char *command = "./nonexistentcommand";  // Replace with a non-existent command
    char *args[] = {command, "Hello, World!", NULL};

    int result = execv(command, args);
    if (result == -1) {
        printf("Error: execv failed as expected with an invalid command.\n");
    } else {
        // Should not reach here if execv handles errors correctly
        printf("Exec succeeded unexpectedly with result: %d\n", result);
    }
    exit(1);
}