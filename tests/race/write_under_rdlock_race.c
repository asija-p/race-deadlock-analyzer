#include <pthread.h>
int x;
pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;

void* worker(void* arg) {
    pthread_rwlock_rdlock(&rw);
    x = 5;      // BAG: pise pod READ lockom
    pthread_rwlock_unlock(&rw);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_rwlock_rdlock(&rw);
    x = 10;     // i ovde isti BAG
    pthread_rwlock_unlock(&rw);
    return 0;
}