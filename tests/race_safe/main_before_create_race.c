#include <pthread.h>

int x;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    x = 10;   // main pise x STROGO PRE nego sto je worker uopste kreiran

    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);

    return 0;
}