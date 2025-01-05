#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
static int ready = 0;

void* producer_thread(void* arg) {
    int count = 0;
    while (1) {
        sleep(1); 
        pthread_mutex_lock(&lock);

        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        ready = 1;
        count++;
        printf("[Producer] Sent event #%d\n", count);

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    int count = 0;
    while (1) {
        pthread_mutex_lock(&lock);
        while (ready == 0) {
            pthread_cond_wait(&cond, &lock);
        }
        ready = 0;
        count++;
        printf("[Consumer] Handled event #%d\n", count);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t producer, consumer;
    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    return 0;
}
