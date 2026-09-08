#include <pthread.h>
struct Data { int val; };
struct Data d;   // globalna, deljena

void* worker(void* arg) { d.val = 5; return NULL; }

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);

    struct Data d;   // LOKALNA, senci globalnu - druga promenljiva!
    d.val = 10;

    pthread_join(t, NULL);
    return 0;
}
