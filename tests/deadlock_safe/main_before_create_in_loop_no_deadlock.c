#include <pthread.h>

pthread_mutex_t m1, m2;
pthread_t t;
int i;

void* worker(void* arg) {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

int main() {
    // main-ov konfliktni deo je STROGO PRE petlje koja kreira niti -
    // worker JOS NE POSTOJI (ni jedna instanca) u ovom trenutku.
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);

    for (i = 0; i < 3; i++) {
        pthread_create(&t, NULL, worker, NULL);
    }

    return 0;
}