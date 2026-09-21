#include <pthread.h>

int x;

void* worker1(void* arg) {
    x++;
    return NULL;
}

void* worker2(void* arg) {
    --x;
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker1, NULL);
    pthread_create(&t2, NULL, worker2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}