#include <pthread.h>

pthread_mutex_t lock1;
pthread_mutex_t lock2;

void* thread1_func(void* arg) {
    int condition = 1;

    pthread_mutex_lock(&lock1);

    if (condition) {
        pthread_mutex_lock(&lock2);
        pthread_mutex_unlock(&lock2);
    }

    pthread_mutex_unlock(&lock1);
    return NULL;
}

int main() {
    pthread_mutex_init(&lock1, NULL);
    pthread_mutex_init(&lock2, NULL);

    pthread_t t1;
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_join(t1, NULL);

    pthread_mutex_destroy(&lock1);
    pthread_mutex_destroy(&lock2);
    return 0;
}