#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void thread1() {
    int i;
    for (i = 0; i < 5; i++) {
        pthread_mutex_lock(&m1);
        pthread_mutex_unlock(&m1);
    }
}

int main() {
    pthread_mutex_init(&m1, NULL);
    pthread_t t;
    pthread_create(&t, NULL, (void*(*)(void*))thread1, NULL);
    pthread_join(t, NULL);
    pthread_mutex_destroy(&m1);
    return 0;
}