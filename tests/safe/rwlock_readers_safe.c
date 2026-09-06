#include <pthread.h>

pthread_rwlock_t rw1 = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_func(void* arg) {
    pthread_mutex_lock(&m2);
    pthread_rwlock_rdlock(&rw1);
    pthread_rwlock_unlock(&rw1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, thread_func, NULL);

    pthread_rwlock_rdlock(&rw1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_rwlock_unlock(&rw1);

    return 0;
}