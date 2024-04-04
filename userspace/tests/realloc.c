//
// Created by mirza on 14.01.24.
//

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

void manual_strcpy(char *dest, const char *src) {
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
}

// Function to compare two strings.
int manual_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void test_normal_allocation() {
    char *original = malloc(10);
    manual_strcpy(original, "Test");

    char *resized = realloc(original, 20);
    assert(resized != NULL);
    assert(manual_strcmp(resized, "Test") == 0);

    free(resized);
}

void test_shrinking_memory() {
    char *original = malloc(20);
    manual_strcpy(original, "Test");

    char *shrunk = realloc(original, 10);
    assert(shrunk != NULL);
    assert(manual_strcmp(shrunk, "Test") == 0);

    free(shrunk);
}

void test_null_pointer() {
    char *allocated = realloc(NULL, 10);
    assert(allocated != NULL);

    free(allocated);
}

void test_zero_size() {
    char *original = malloc(10);
    manual_strcpy(original, "Test");

    char *resized = realloc(original, 0);

    free(resized);
}

int main() {
    test_normal_allocation();
    test_shrinking_memory();
    test_null_pointer();
    test_zero_size();

    printf("All tests passed!\n");
    return 0;
}