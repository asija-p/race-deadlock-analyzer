#include <pthread.h>
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* threadA(void* arg) {
    pthread_mutex_t lock;
    pthread_mutex_lock(&lock);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&lock);
    return NULL;
}

void* threadB(void* arg) {
    pthread_mutex_t lock;
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t ta, tb;
    pthread_create(&ta, NULL, threadA, NULL);
    pthread_create(&tb, NULL, threadB, NULL);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    return 0;
}
