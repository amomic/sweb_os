#include "stdio.h"
#include "stdlib.h"

int main() {
    int n = 5, i, *ptr, sum = 0;


    ptr = (int *) malloc(n * sizeof(int));

    if (ptr == NULL) {
        printf("Error!\n");
        exit(0);
    }

    for (i = 0; i < n; ++i) {
        *(ptr + i) = 1;
        sum += *(ptr + i);
    }

    printf("Sum = %d \n", sum);
    free(ptr);

    return 0;
}