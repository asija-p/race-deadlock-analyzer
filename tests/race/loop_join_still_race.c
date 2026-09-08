#include <pthread.h>

int x;
pthread_t t;
int i;

void* worker(void* arg) {
    x = 5;
    return NULL;
}

int main() {
    for (i = 0; i < 3; i++) {
        pthread_create(&t, NULL, worker, NULL);
    }
    pthread_join(t, NULL);   // ceka SAMO poslednju instancu (t je prepisan)

    x = 10;   // da li su prve dve instance zaista "gotove"? NE ZNAMO SIGURNO

    return 0;
}