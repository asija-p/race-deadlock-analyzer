#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2, m3;

void thread1(int x) {
    switch (x) {
        case 0:
            pthread_mutex_lock(&m1);
            break;
        case 1:
            pthread_mutex_lock(&m2);
            break;
        default:
            pthread_mutex_lock(&m3);
            break;
    }
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
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
    pthread_mutex_init(&m3, NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    pthread_mutex_destroy(&m3);
    return 0;
}