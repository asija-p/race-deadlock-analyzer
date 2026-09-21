#include <pthread.h>

void* worker1(void* arg) {
    static int n;
    n = 1;
    return NULL;
}

void* worker2(void* arg) {
    static int n;
    n = 2;
    return NULL;
}

int main() {
    pthread_t a, b;
    pthread_create(&a, NULL, worker1, NULL);
    pthread_create(&b, NULL, worker2, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}