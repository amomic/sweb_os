//
// Created by mirza on 14.01.24.
//

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

int is_memory_zeroed(const char *mem, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (mem[i] != 0) {
            return 0; // Memory block is not all zeros
        }
    }
    return 1; // Memory block is all zeros
}

void test_calloc() {
    size_t num = 10;
    size_t size = sizeof(char);
    char *block = (char *)calloc(num, size);
    assert(block != NULL);
    assert(is_memory_zeroed(block, num * size));

    free(block);
}

void test_calloc_zero_count() {
    size_t num = 0;
    size_t size = sizeof(char);
    char *block = (char *)calloc(num, size);

    free(block);
}

void test_calloc_zero_size() {
    size_t num = 10;
    size_t size = 0;
    char *block = (char *)calloc(num, size);

    free(block);
}

/*
void test_calloc_large() {
    size_t num = (1 << 30); // 1 Gigabyte
    size_t size = sizeof(char);
    char *block = (char *)calloc(num, size);

    assert(block == NULL);

    free(block);
}*/

int main() {
    test_calloc();
    test_calloc_zero_count();
    test_calloc_zero_size();
    // test_calloc_large();

    printf("All tests passed!\n");
    return 0;
}
