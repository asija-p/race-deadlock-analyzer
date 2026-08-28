#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void thread1() {
    int i;
    for (i = 0; i < 5; i++) {
        pthread_mutex_lock(&m1);
        pthread_mutex_unlock(&m1);
    }
}