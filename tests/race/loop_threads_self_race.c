#include <pthread.h>

int x;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    pthread_t t[4];
    int i;
    for (i = 0; i < 4; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (i = 0; i < 4; i++) pthread_join(t[i], NULL);
    return 0;
}