#include <pthread.h>

int x;

void* workerA(void* arg) {
    x = 5;
    return NULL;
}

void* workerB(void* arg) {
    x = 20;
    return NULL;
}

int main() {
    pthread_t ta, tb;
    pthread_create(&ta, NULL, workerA, NULL);
    pthread_create(&tb, NULL, workerB, NULL);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    return 0;
}