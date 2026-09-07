#include <pthread.h>

pthread_mutex_t g;
pthread_mutex_t m1, m2, m3;

void* thread1(void* arg) {
    pthread_mutex_lock(&g);
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&g);
    return NULL;
}

void* thread2(void* arg) {
    pthread_mutex_lock(&g);
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m3);
    pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&g);
    return NULL;
}

void* thread3(void* arg) {
    pthread_mutex_lock(&m3);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m3);
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_create(&t3, NULL, thread3, NULL);
    return 0;
}