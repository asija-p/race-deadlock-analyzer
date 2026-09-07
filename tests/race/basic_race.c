#include <pthread.h>

int x;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    x = 10;
    return 0;
}