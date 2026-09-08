#include <pthread.h>
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_t t;
int cond = 0;

void* workerA(void* arg) {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void* workerB(void* arg) {
    return NULL;
}

int main() {
    if (cond) {
        pthread_create(&t, NULL, workerA, NULL);
    } else {
        pthread_create(&t, NULL, workerB, NULL);
    }
    pthread_join(t, NULL);

    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);

    return 0;
}
