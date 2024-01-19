#include "../libc/include/stdio.h"

#define MB 1024*1024

int array[MB];

int main() {

    for (int iter = 0; iter < MB; iter += 4096)
        array[iter] = 1;

    fork();

    for (int iter = 0; iter < MB; iter += 4096)
        array[iter] += 1;

    fork();

    for (int iter = 0; iter < MB; iter += 4096)
        array[iter] += 1;

    sleep(5);
    return 0;
}