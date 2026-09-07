#include <pthread.h>

int x;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int cond = 0;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);

    if (cond) {
        pthread_mutex_lock(&m);
    }
    x = 10;
    if (cond) {
        pthread_mutex_unlock(&m);
    }

    return 0;
}