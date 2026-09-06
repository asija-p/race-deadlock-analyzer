#include <pthread.h>
#include <stddef.h>

pthread_mutex_t m1, m2;

void B() {
    pthread_t t;
    pthread_create(&t, NULL, (void*(*)(void*))worker, NULL);  // B sam NIJE rekurzivan
}

void worker() {
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
}

void A(int depth) {
    if (depth > 0) {
        A(depth - 1);
    } else {
        B();
    }
}

int main() {
    pthread_mutex_init(&m1, NULL);
    A(2);
    return 0;
}