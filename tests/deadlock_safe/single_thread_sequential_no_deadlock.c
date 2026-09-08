#include <pthread.h>

pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m1    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2    = PTHREAD_MUTEX_INITIALIZER;

void certain_path() {
    pthread_mutex_lock(&guard);
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&guard);
}

void uncertain_path(int flag) {
    pthread_mutex_lock(&guard);
    if (flag) {
        pthread_mutex_lock(&m1);
    }
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    if (flag) {
        pthread_mutex_unlock(&m1);
    }
    pthread_mutex_unlock(&guard);
}

void closer() {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&guard);
    pthread_mutex_unlock(&guard);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
}

int main() {
    certain_path();
    uncertain_path(1);
    closer();
    return 0;
}