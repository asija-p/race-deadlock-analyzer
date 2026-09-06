#include <pthread.h>

pthread_rwlock_t rw1 = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rw2 = PTHREAD_RWLOCK_INITIALIZER;

void* thread_func(void* arg) {
    pthread_rwlock_wrlock(&rw2);
    pthread_rwlock_wrlock(&rw1);
    pthread_rwlock_unlock(&rw1);
    pthread_rwlock_unlock(&rw2);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, thread_func, NULL);

    pthread_rwlock_wrlock(&rw1);
    pthread_rwlock_wrlock(&rw2);
    pthread_rwlock_unlock(&rw2);
    pthread_rwlock_unlock(&rw1);

    return 0;
}