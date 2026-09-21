#include <pthread.h>

int arr[2];

void* worker(void* arg) {
    arr[0] = 1;
    return NULL;
}

int main() {
    pthread_t t;
    int i = 0;
    pthread_create(&t, NULL, worker, NULL);
    arr[i] = 2;
    pthread_join(t, NULL);
    return 0;
}