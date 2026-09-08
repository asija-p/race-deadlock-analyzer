#include <pthread.h>
int x;   // globalna, deljena

void* worker(void* arg) { x = 5; return NULL; }

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);

    int x = 10;   // LOKALNA, senci globalnu - druga promenljiva!
    x = 20;

    pthread_join(t, NULL);
    return 0;
}