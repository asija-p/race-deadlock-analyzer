#include <pthread.h>

pthread_rwlock_t rw_common = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_func(void* arg) {
    pthread_rwlock_rdlock(&rw_common);
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    pthread_rwlock_unlock(&rw_common);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, thread_func, NULL);

    pthread_rwlock_rdlock(&rw_common);
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    pthread_rwlock_unlock(&rw_common);

    return 0;
}