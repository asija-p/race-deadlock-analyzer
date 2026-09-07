#include <pthread.h>

pthread_mutex_t m1;
pthread_mutex_t m2;

// Namerno ne radi nista sa bravama - postoji samo da izazove OBICAN
// (ne-pthread) interproceduralni poziv IZMEDJU dva lock() poziva u main-u,
// u ISTOM CFG bloku (bez grananja izmedju njih).
//
// Regresioni test za bag: BlockBuffer/PairsAtBlock su clanske promenljive
// DeadlockVisitor-a, a ISTI Visitor objekat se deli kroz CELU
// interproceduralnu rekurziju. Bez EnterNestedCall/ExitNestedCall
// izolacije, BeginBlock koji se poziva pri ulasku u do_nothing() bi
// obrisao BlockBuffer POZIVAOCA (main-a) - a taj bafer u tom trenutku vec
// sadrzi m1->m2 ivicu generisanu linijom ispod. Kako se posle poziva ne
// zakljucava nijedna nova brava (samo unlock), ta ivica se NE regenerise -
// ako je bag prisutan, gubi se zauvek i deadlock se ne prijavljuje.
void do_nothing() {
    int x = 1;
    x = x + 1;
}

void* thread2_func(void* arg) {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t t2;
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);   // generise LockPair m1->m2, ide u BlockBuffer
    do_nothing();               // ugnjezdeni poziv U ISTOM BLOKU - kriticna tacka bag-a
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);

    pthread_join(t2, NULL);
    return 0;
}