#include <pthread.h>
#include <stddef.h>

pthread_mutex_t locks[3];

void thread1() {
    pthread_mutex_lock(&locks[0]);
    pthread_mutex_lock(&locks[1]);
    pthread_mutex_unlock(&locks[1]);
    pthread_mutex_unlock(&locks[0]);
}

void thread2() {
    pthread_mutex_lock(&locks[1]);
    pthread_mutex_lock(&locks[0]);
    pthread_mutex_unlock(&locks[0]);
    pthread_mutex_unlock(&locks[1]);
}

int main() {
    pthread_mutex_init(&locks[0], NULL);
    pthread_mutex_init(&locks[1], NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&locks[0]);
    pthread_mutex_destroy(&locks[1]);
    return 0;
}