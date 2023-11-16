#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    printf("Test: Termination by Signal\n");
    size_t pid = fork();
    if (pid == 0)
    {
        // Child process
        printf("Child process (PID %ld) is running.\n", pid);
        // Simulate some work in the child process
        sleep(2);
        printf("Child process (PID %ld) is exiting.\n", pid);
        return -10;
    } else
    {
        int status;
        printf("Prewait\n");
        sleep(5);
        size_t terminated_pid = 0;
        printf("%zu\n", terminated_pid);
        terminated_pid = waitpid(pid, &status, 0);
        printf("%d\n", status);

        if (status == -10)
        {
            printf("Test Passed: Child process (PID %ld) terminated by signal %d.\n", terminated_pid, status);
        } else
        {
            printf("Test Failed: Child process did not terminate by signal.\n");
        }
    }
    return 0;
}
