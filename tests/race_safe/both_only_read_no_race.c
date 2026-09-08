#include <pthread.h>
int x;
int yw;
int ym;

void* worker(void* arg) { yw = x; return NULL; }   // samo cita x (lokalno yw)

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    ym = x;   // i main samo cita x (lokalno ym)
    pthread_join(t, NULL);
    return 0;
}
