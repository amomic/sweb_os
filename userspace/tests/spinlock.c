//
// Created by mirza on 08.01.24.
//

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

#define NUM_OF_THREADS         20
#define WAITING_TIME        1024*1024*500

pthread_spinlock_t global_lock;
size_t global;

void test_mutex(){
    pthread_spin_lock(&global_lock);
    assert(!global && "spin lock error");
    global = 1;
    for(size_t i = 0; i < WAITING_TIME; ++i);
    global = 0;
    pthread_spin_unlock(&global_lock);
}

int main()
{
    int var = pthread_spin_init(&global_lock, NULL);
    assert(!var && "spinlock init unsuccessfull");

    pthread_t tid[NUM_OF_THREADS];
    for(size_t i = 0; i < NUM_OF_THREADS; ++i)
    {
        pthread_create(tid + i, NULL, (void*(*)(void*))test_mutex, NULL);
    }

    return 0;
}
