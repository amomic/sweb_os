//
// Created by mirza on 07.11.23.
// Too many args

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

int main() {
    printf("Test with Too many args - should not execute\n");
    char *command = "/usr/exechelper.sweb";
    char *args[] = {command, "arg1", "arg2", "arg3", "arg4", "arg5", "arg6", "arg7", "arg8", "arg9", "arg10", "arg11",
                    "arg12", "arg13", "arg14", "arg15", "arg16", "arg17", NULL};

    int result = execv(command, args);

    assert(result == -1 && "Exec should fail as expected due to too many arguments");
}