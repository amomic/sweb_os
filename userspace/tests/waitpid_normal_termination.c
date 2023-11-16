#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main() {
    printf("Test: Normal Termination\n");
    size_t pid = fork();
    if (pid == 0) {
        // Child process
        printf("Child process (PID %zu) is running.\n", pid);
        // Simulate some work in the child process
        sleep(2);
        printf("Child process (PID %zu) is exiting.\n", pid);
        exit(0);
    }
    else
    {
        sleep(5);
        int status;
        size_t terminated_pid = waitpid(pid, &status, 0);

        printf("%d \n", status);
        if (status == 0)
        {
            printf("Test Passed: Child process (PID %zu) exited with status %d.\n", terminated_pid, status);
        } else
        {
            printf("Test Failed: Child process did not exit normally.\n");
        }
    }
    return 0;
}
