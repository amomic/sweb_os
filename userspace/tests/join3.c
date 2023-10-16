//
// Created by mirza on 16.10.23.
// Thread joining on itself
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

int main(){
    assert(pthread_join(1, NULL));
    return 0;
}