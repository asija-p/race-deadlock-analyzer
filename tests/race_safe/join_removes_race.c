#include <pthread.h>

int x;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_join(t, NULL);   // main CEKA da worker zavrsi

    x = 10;   // main-ov pristup POSLE join-a - worker je SIGURNO gotov

    return 0;
}