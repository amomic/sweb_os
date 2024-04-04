//
// Created by mirza on 13.01.24.
//

#include "stdlib.h"
#include "stdio.h"
#include "assert.h"

#define ARRAY_SIZE_1 10
#define ARRAY_SIZE_2 50
#define CONST_VAL_1 0xABC
#define CONST_VAL_2 0xDEF

int main() {
    int *intArray = (int *) malloc(ARRAY_SIZE_1 * sizeof(int));
    assert(intArray != NULL && "Memory allocation for intArray failed");

    double *doubleArray = (double *) malloc(ARRAY_SIZE_2 * sizeof(double));
    assert(doubleArray != NULL && "Memory allocation for doubleArray failed");

    for (int i = 0; i < ARRAY_SIZE_1; i++) {
        intArray[i] = CONST_VAL_1;
    }
    for (int i = 0; i < ARRAY_SIZE_2; i++) {
        doubleArray[i] = CONST_VAL_2;
    }

    for (int i = 0; i < ARRAY_SIZE_1; i++) {
        assert(intArray[i] == CONST_VAL_1 && "Data integrity check failed for intArray");
    }
    for (int i = 0; i < ARRAY_SIZE_2; i++) {
        assert(doubleArray[i] == CONST_VAL_2 && "Data integrity check failed for doubleArray");
    }

    // Free  memory
    free(intArray);
    free(doubleArray);

    // Test reallocating after freeing
    char *charArray = (char *) malloc(ARRAY_SIZE_1 * sizeof(char));
    assert(charArray != NULL && "Memory allocation for charArray failed");

    free(charArray);

    printf("All tests passed successfully.\n");

    return 0;
}