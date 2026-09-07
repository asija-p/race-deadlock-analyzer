#include <pthread.h>

struct Resource {
    pthread_mutex_t lock1;
    pthread_mutex_t lock2;
};

struct Resource res = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

void* thread_func(void* arg) {
    struct Resource *p = &res;
    pthread_mutex_lock(&p->lock2);
    pthread_mutex_lock(&p->lock1);
    pthread_mutex_unlock(&p->lock1);
    pthread_mutex_unlock(&p->lock2);
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, thread_func, NULL);

    struct Resource *p = &res;
    pthread_mutex_lock(&(*p).lock1);
    pthread_mutex_lock(&(*p).lock2);
    pthread_mutex_unlock(&(*p).lock2);
    pthread_mutex_unlock(&(*p).lock1);

    return 0;
}