#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    printf("Test: Waiting for a Specific Child\n");
    size_t pid = fork();
    printf("%zu", pid);
    if (pid == 0)
    {
        // Child process
        printf("Child process (PID %ld) is running.\n", pid);
        // Simulate some work in the child process
        sleep(2);
        printf("Child process (PID %ld) is exiting.\n", pid);
        exit(0);
    }
    size_t child_pid = fork();

    printf("2nd proc %zu\n", child_pid);


    if (child_pid == 4)
    {
        // Child process
        printf("Child process (PID %ld) is running.\n", child_pid);
        // Simulate some work in the child process
        sleep(2);
        printf("Child process (PID %ld) is exiting.\n", child_pid);
        exit(EXIT_SUCCESS);
    }


    int status;
    sleep(10);
    size_t terminated_pid = waitpid(child_pid, &status, 0);

    if (terminated_pid == child_pid && status == EXIT_SUCCESS)
    {
        printf("Test Passed: Child process (PID %ld) exited with status %d.\n", terminated_pid, status);
    } else
    {
        printf("Test Failed: Incorrect child process termination.\n");
    }

    return 0;
}
