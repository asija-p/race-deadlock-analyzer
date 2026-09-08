#include <pthread.h>
int x;
int cond = 0;

void* worker(void* arg) { x = 5; return NULL; }

int main() {
    pthread_t t;
    if (cond) {
        pthread_create(&t, NULL, worker, NULL);
        x = 10;   // AKO se ovo uopste izvrsi, worker je SIGURNO vec kreiran
    }
    return 0;
}
