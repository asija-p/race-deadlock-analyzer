#include <pthread.h>

int x;
int i;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    pthread_t t;
    for (i = 0; i < 2; i++) {
        pthread_create(&t, NULL, worker, NULL);
    }
    x = 10;
    return 0;
}