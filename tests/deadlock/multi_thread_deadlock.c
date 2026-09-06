#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;
int flag_a = 1;
int flag_b = 0;

void worker(int flag) {
    if (flag) {
        pthread_mutex_lock(&m1);
        pthread_mutex_lock(&m2);
        pthread_mutex_unlock(&m2);
        pthread_mutex_unlock(&m1);
    } else {
        pthread_mutex_lock(&m2);
        pthread_mutex_lock(&m1);
        pthread_mutex_unlock(&m1);
        pthread_mutex_unlock(&m2);
    }
}

void worker_wrapper() {
    worker(1);
}

int main() {
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);
    pthread_t t[2];
    int i;
    for (i = 0; i < 2; i++) {
        pthread_create(&t[i], NULL, (void*(*)(void*))worker_wrapper, NULL);
    }
    for (i = 0; i < 2; i++) {
        pthread_join(t[i], NULL);
    }
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    return 0;
}