#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void f1() {
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
}

void f2() {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
}

int main() {
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);
    f1();
    f2();
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    return 0;
}