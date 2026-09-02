#include <pthread.h>
#include <stddef.h>

pthread_mutex_t locks[3];

void recursive_lock(int depth) {
    if (depth < 3) {
        pthread_mutex_lock(&locks[depth]);
        recursive_lock(depth + 1);
        pthread_mutex_unlock(&locks[depth]);
    }
}

void thread1() {
    recursive_lock(0);
}

void thread2() {
    pthread_mutex_lock(&locks[2]);
    pthread_mutex_lock(&locks[0]);
    pthread_mutex_unlock(&locks[0]);
    pthread_mutex_unlock(&locks[2]);
}

int main() {
    pthread_mutex_init(&locks[0], NULL);
    pthread_mutex_init(&locks[1], NULL);
    pthread_mutex_init(&locks[2], NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*(*)(void*))thread1, NULL);
    pthread_create(&t2, NULL, (void*(*)(void*))thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&locks[0]);
    pthread_mutex_destroy(&locks[1]);
    pthread_mutex_destroy(&locks[2]);
    return 0;
}