#include <pthread.h>
#include <stddef.h>

typedef struct {
    pthread_mutex_t lock1;
    pthread_mutex_t lock2;
} Resource;

Resource res;

void thread1() {
    pthread_mutex_lock(&res.lock1);
    pthread_mutex_lock(&res.lock2);
    pthread_mutex_unlock(&res.lock2);
    pthread_mutex_unlock(&res.lock1);
}

void thread2() {
    pthread_mutex_lock(&res.lock2);
    pthread_mutex_lock(&res.lock1);
    pthread_mutex_unlock(&res.lock1);
    pthread_mutex_unlock(&res.lock2);
}

int main() {
    pthread_mutex_init(&res.lock1, NULL);
    pthread_mutex_init(&res.lock2, NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&res.lock1);
    pthread_mutex_destroy(&res.lock2);
    return 0;
}