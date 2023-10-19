//
// Created by mirza on 19.10.23.
//

#include "stdio.h"
#include "assert.h"
#include "pthread.h"

void* thread_function(void* arg) {
    return NULL;
}

int main(){
    size_t thread;
    pthread_create(&thread, NULL, thread_function, NULL);
    printf("DETACHED: %d",pthread_detach(thread));
}