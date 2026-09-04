#include <pthread.h>
#include <stddef.h>

pthread_mutex_t a1, a2, b1, b2;

void thread1() {
    pthread_mutex_lock(&a1);
    pthread_mutex_lock(&a2);
    pthread_mutex_unlock(&a2);
    pthread_mutex_unlock(&a1);
}

void thread2() {
    pthread_mutex_lock(&a2);
    pthread_mutex_lock(&a1);
    pthread_mutex_unlock(&a1);
    pthread_mutex_unlock(&a2);
}

void thread3() {
    pthread_mutex_lock(&b1);
    pthread_mutex_lock(&b2);
    pthread_mutex_unlock(&b2);
    pthread_mutex_unlock(&b1);
}

void thread4() {
    pthread_mutex_lock(&b2);
    pthread_mutex_lock(&b1);
    pthread_mutex_unlock(&b1);
    pthread_mutex_unlock(&b2);
}

int main() {
    pthread_mutex_init(&a1, NULL);
    pthread_mutex_init(&a2, NULL);
    pthread_mutex_init(&b1, NULL);
    pthread_mutex_init(&b2, NULL);
    pthread_t t1, t2, t3, t4;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_create(&t3, NULL, (void*(*)(void*))thread3, NULL);
    pthread_create(&t4, NULL, (void*(*)(void*))thread4, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_mutex_destroy(&a1);
    pthread_mutex_destroy(&a2);
    pthread_mutex_destroy(&b1);
    pthread_mutex_destroy(&b2);
    return 0;
}