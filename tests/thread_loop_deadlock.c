#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void worker() {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
}

int main() {
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);

    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);

    pthread_t t[3];
    int i;
    for (i = 0; i < 3; i++) {
        pthread_create(&t[i], NULL, (void*(*)(void*))worker, NULL);
    }
    for (i = 0; i < 3; i++) {
        pthread_join(t[i], NULL);
    }
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    return 0;
}