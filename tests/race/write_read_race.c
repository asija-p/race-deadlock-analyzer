#include <pthread.h>
int x, y;

void* worker(void* arg) { x = 5; return NULL; }

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    y = x;   // citanje x - trebalo bi sad da bude race
    pthread_join(t, NULL);
    return 0;
}