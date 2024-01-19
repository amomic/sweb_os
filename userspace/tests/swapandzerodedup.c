
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#define SIZE 4096
#define MB 1024ULL * 1024ULL
size_t var[10 * MB];

int main() {

    for (size_t i = 0; i < 10 * MB; ++i) {
        if (!(i % 4096)) {
            var[i] = i - 1;
        }
    }


    sleep(2);

    int array[SIZE];

    sleep(2);
    for (int i = 0; i < SIZE; ++i) {
        if (i == SIZE / 2) {
            array[i] = 5;
        }

        printf("%d", array[i]);
    }
    sleep(2);

    printf("\nDone\n");

    return 0;
}

