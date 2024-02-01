#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

//fill stack with smth
void fillStack(char *buffer, int depth, int maxDepth) {
    if (depth < maxDepth) {
        buffer[depth] = (char)(depth % 256);
        fillStack(buffer, depth + 1, maxDepth);
    }
}

// check if anything inside
int verifyStack(char *buffer, int maxDepth) {
    for (int i = 0; i < maxDepth; ++i) {
        if (buffer[i] != (char)(i % 256)) {
            printf("stack  failed at %d\n", i);
            return -1;
        }
    }
    return 0;
}

int main() {
    const int stackSize = 1024;
    char stackBuffer[stackSize];

    printf("Filling stack...\n");
    fillStack(stackBuffer, 0, stackSize);

    printf("Calling swapStack syscall...\n");
    swapStack();

    printf("Verifying stack contents...\n");
    if (verifyStack(stackBuffer, stackSize) == 0) {
        printf(" swapStack done!\n");
    } else {
        printf("Error\n");
    }

    return 0;
}
