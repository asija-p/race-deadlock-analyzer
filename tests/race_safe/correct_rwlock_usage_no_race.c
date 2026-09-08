#include <pthread.h>
int x;
pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;

void* reader(void* arg) {
    pthread_rwlock_rdlock(&rw);
    int local = x;   // citanje pod rdlock
    pthread_rwlock_unlock(&rw);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, reader, NULL);
    pthread_rwlock_wrlock(&rw);
    x = 10;   // pisanje pod wrlock
    pthread_rwlock_unlock(&rw);
    pthread_join(t, NULL);
    return 0;
}
