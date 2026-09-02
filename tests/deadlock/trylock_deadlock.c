#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void thread1() {
    pthread_mutex_lock(&m1);
    pthread_mutex_trylock(&m2);
    pthread_mutex_unlock(&m1);
}

void thread2() {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
}

int main() {
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    return 0;
}