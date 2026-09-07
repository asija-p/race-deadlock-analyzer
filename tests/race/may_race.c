#include <pthread.h>

int x;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int cond = 0;

void* worker(void* arg) {
    x = 5;   // nema nikakve brave
    return NULL;
}

int main() {
    pthread_t t;

    if (cond) {
        pthread_create(&t, NULL, worker, NULL);   // worker se MOŽDA kreira
    }

    x = 10;   // nema nikakve brave

    if (cond) {
        pthread_join(t, NULL);
    }

    return 0;
}