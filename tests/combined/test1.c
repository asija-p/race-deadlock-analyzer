#include <pthread.h>

pthread_mutex_t a1, a2;
pthread_mutex_t b1, b2, b3;
pthread_mutex_t safe1, safe2, safe3;

int x;
int y;
int cond;

/* --- Deadlock klaster 1: a1 <-> a2 --- */
void* threadA1(void* arg) {
    pthread_mutex_lock(&a1);
    pthread_mutex_lock(&a2);
    pthread_mutex_unlock(&a2);
    pthread_mutex_unlock(&a1);
    return NULL;
}

void* threadA2(void* arg) {
    pthread_mutex_lock(&a2);
    pthread_mutex_lock(&a1);
    pthread_mutex_unlock(&a1);
    pthread_mutex_unlock(&a2);
    return NULL;
}

/* --- Deadlock klaster 2: b1 -> b2 -> b3 -> b1 (trosmerni ciklus) --- */
void* threadB1(void* arg) {
    pthread_mutex_lock(&b1);
    pthread_mutex_lock(&b2);
    pthread_mutex_unlock(&b2);
    pthread_mutex_unlock(&b1);
    return NULL;
}

void* threadB2(void* arg) {
    pthread_mutex_lock(&b2);
    pthread_mutex_lock(&b3);
    pthread_mutex_unlock(&b3);
    pthread_mutex_unlock(&b2);
    return NULL;
}

void* threadB3(void* arg) {
    pthread_mutex_lock(&b3);
    pthread_mutex_lock(&b1);
    pthread_mutex_unlock(&b1);
    pthread_mutex_unlock(&b3);
    return NULL;
}

/* --- Bezopasni parovi (uvek isti redosled, nema ciklusa) --- */
void* threadSafe(void* arg) {
    pthread_mutex_lock(&safe1);
    pthread_mutex_lock(&safe2);
    pthread_mutex_lock(&safe3);
    pthread_mutex_unlock(&safe3);
    pthread_mutex_unlock(&safe2);
    pthread_mutex_unlock(&safe1);
    return NULL;
}

/* --- MUST-RACE: bez ikakve zastite, nema join --- */
void* threadMustRace(void* arg) {
    x = 5;
    return NULL;
}

/* --- MAY-RACE: uslovno kreiranje niti (isti obrazac kao ranije) --- */
void* threadMayRace(void* arg) {
    y = 10;
    return NULL;
}

int main() {
    pthread_t ta1, ta2, tb1, tb2, tb3, tsafe, tmust, tmay;

    pthread_create(&ta1, NULL, threadA1, NULL);
    pthread_create(&ta2, NULL, threadA2, NULL);

    pthread_create(&tb1, NULL, threadB1, NULL);
    pthread_create(&tb2, NULL, threadB2, NULL);
    pthread_create(&tb3, NULL, threadB3, NULL);

    pthread_create(&tsafe, NULL, threadSafe, NULL);

    pthread_create(&tmust, NULL, threadMustRace, NULL);
    x = 20;

    if (cond) {
        pthread_create(&tmay, NULL, threadMayRace, NULL);
    }
    y = 30;

    return 0;
}