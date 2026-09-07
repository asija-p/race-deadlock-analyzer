#include <pthread.h>

int x;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg) {
    pthread_mutex_lock(&m);
    x = 5;
    pthread_mutex_unlock(&m);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_mutex_lock(&m);
    x = 10;
    pthread_mutex_unlock(&m);
    return 0;
}