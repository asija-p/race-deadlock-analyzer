#include <pthread.h>
int x;
void* worker(void* arg) { x = 5; return NULL; }
int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    if (x > 0) { }   // citanje x u uslovu
    pthread_join(t, NULL);
    return 0;
}