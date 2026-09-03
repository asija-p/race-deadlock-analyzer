#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void inner_lock(pthread_mutex_t *m) {
    pthread_mutex_lock(m);
}

void outer_lock(pthread_mutex_t *m) {
    inner_lock(m);
}

void thread1() {
    outer_lock(&m1);
    outer_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
}

void thread2() {
    outer_lock(&m2);
    outer_lock(&m1);
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