#include <pthread.h>

int x;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* w1(void* arg) {
    pthread_mutex_lock(&m);
    x = 1;
    pthread_mutex_unlock(&m);
    return NULL;
}

void* w2(void* arg) {
    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);
    pthread_mutex_lock(&m);
    x = 2;
    pthread_mutex_unlock(&m);
    return NULL;
}

int main() {
    pthread_t a, b;
    pthread_create(&a, NULL, w1, NULL);
    pthread_create(&b, NULL, w2, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}