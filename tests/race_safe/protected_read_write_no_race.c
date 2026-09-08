#include <pthread.h>
int x;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int y;

void* worker(void* arg) {
    pthread_mutex_lock(&m);
    y = x;      // citanje x, pod bravom
    pthread_mutex_unlock(&m);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_mutex_lock(&m);
    x = 10;     // pisanje x, pod ISTOM bravom
    pthread_mutex_unlock(&m);
    pthread_join(t, NULL);
    return 0;
}