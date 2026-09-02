#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void my_lock(pthread_mutex_t *m) {
    pthread_mutex_lock(m);
}

void my_unlock(pthread_mutex_t *m) {
    pthread_mutex_unlock(m);
}

void thread1() {
    my_lock(&m1);
    my_lock(&m2);
    my_unlock(&m2);
    my_unlock(&m1);
}

void thread2() {
    my_lock(&m2);
    my_lock(&m1);
    my_unlock(&m1);
    my_unlock(&m2);
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