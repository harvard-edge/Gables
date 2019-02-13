#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include <unistd.h>

#include "Barrier.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int counter = 0;

int decrement() {

    if (pthread_mutex_lock(&lock) != 0) {
        perror("Failed to take lock.");
        return -1;
    }

    counter--;

    if (pthread_mutex_unlock(&lock) != 0) {
        perror("Failed to unlock.");
        return -1;
    }

    return 0;
}

void Barrier_Init(int n_threads) {
    counter = n_threads;
}

int Barrier_Wait() {

    // all threads will decrement the counter
    if (decrement() < 0)
        return -1;

    // all threads except(!) the last one will land in this loop
    while (counter) {
        if (pthread_mutex_lock(&lock) != 0) {
            perror("\n Error in locking mutex");
            return -1;
        }

        // the threads will block on this condition wait
        if (pthread_cond_wait(&cond, &lock) != 0) {
            perror("\n Error in cond wait.");
            return -1;
        }
    }

    // only the last thread will land in here for signaling
    if (0 == counter) {
        if (pthread_mutex_unlock(&lock) != 0) {
            perror("\n Error in locking mutex");
            return -1;
        }

        if (pthread_cond_signal(&cond) != 0) {
            perror("\n Eror while signaling.");
            return -1;
        }
    }

    return 0;
}