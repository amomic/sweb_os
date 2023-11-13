#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int data = 42;
    pid_t pid = fork();

    if (pid == -1) {
        // Error handling for fork failure
        printf("Fork failed");
        exit(-1);
    } else if (pid == 0) {
        // Child process
        printf("Child process - Before changing data: %d\n", data);

        // Change the data in the child process
        data = 84;
        printf("Child process - After changing data: %d\n", data);

        // Child process terminates
        exit(0);
    } else {
        // Parent process
        printf("Parent process - Before child execution: %d\n", data);

        // Wait for the child to terminate
        for (int i = 0; i < 10000; ++i) {

        }

        for (int i = 0; i < 10000; ++i) {

        }

        // Parent process still sees the original data
        printf("Parent process - After child execution: %d\n", data);
    }

    return 0;
}
