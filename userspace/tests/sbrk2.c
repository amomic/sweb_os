#include "stdio.h"
#include "unistd.h"

int main() {
    size_t *buffer = (size_t *) sbrk(55);
    *buffer = 60;
    printf("%zu \n", *buffer);
    return 0;
}