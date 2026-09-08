#include <pthread.h>
int x;
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg) {
    pthread_mutex_lock(&m1);
    x = 5;
    pthread_mutex_unlock(&m1);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_mutex_lock(&m2);   // druga brava - ne stiti
    x = 10;
    pthread_mutex_unlock(&m2);
    return 0;
}