#include <pthread.h>
int x;

void* grandchild(void* arg) { x = 99; return NULL; }

void* child(void* arg) {
    pthread_t tg;
    pthread_create(&tg, NULL, grandchild, NULL);
    pthread_join(tg, NULL);
    return NULL;
}

int main() {
    pthread_t tc;
    pthread_create(&tc, NULL, child, NULL);
    x = 10;   // trci sa grandchild-om - main ne ceka CHILD, a child samo ceka grandchild
    return 0;
}