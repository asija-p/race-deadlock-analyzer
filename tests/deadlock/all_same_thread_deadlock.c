#include <pthread.h>

pthread_mutex_t m1, m2;
pthread_t t;
int flag;

void* worker(void* arg) {
    if (flag % 2 == 0) {
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
    return NULL;
}

int main() {
    int i;
    for (i = 0; i < 2; i++) {
        pthread_create(&t, NULL, worker, NULL);
    }
    return 0;
}