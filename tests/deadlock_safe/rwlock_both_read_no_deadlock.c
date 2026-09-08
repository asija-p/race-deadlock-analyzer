#include <pthread.h>
pthread_rwlock_t rw1 = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rw2 = PTHREAD_RWLOCK_INITIALIZER;

void* thread1(void* arg) {
    pthread_rwlock_rdlock(&rw1);
    pthread_rwlock_rdlock(&rw2);
    pthread_rwlock_unlock(&rw2);
    pthread_rwlock_unlock(&rw1);
    return NULL;
}

void* thread2(void* arg) {
    pthread_rwlock_rdlock(&rw2);
    pthread_rwlock_rdlock(&rw1);
    pthread_rwlock_unlock(&rw1);
    pthread_rwlock_unlock(&rw2);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
