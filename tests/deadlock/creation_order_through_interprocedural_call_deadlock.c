#include <pthread.h>

pthread_mutex_t m1, m2;

void helper() {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
}

void* worker(void* arg) {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    helper();   // main-ov konfliktni deo, POSLE create-a, kroz OBICNU funkciju
    return 0;
}