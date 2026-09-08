#ifndef CONCURRENCYEXCLUSION_H
#define CONCURRENCYEXCLUSION_H

// Osnovno pravilo: PRETPOSTAVLJAMO da su dva pristupa/brave konkurentni
// (mogu da se dese u isto vreme), OSIM ako ne dokažemo suprotno. Ove
// funkcije vraćaju true SAMO kad imamo JASAN dokaz da NISU mogli biti
// konkurentni.

// Prvi dokaz - da je B sigurno završio PRE A (preko pthread_join):
// ako je A već sačekao (joinovao) B, i B nije kreiran u petlji (jer bi
// tad "sačekan" moglo da znači samo JEDNA od više instanci, ne sve).
template <typename T>
bool HasJoinPrecedence(const T &A, const T &B) {
    if (A.JoinedThreads.count(B.ThreadId) && !B.CreatedInLoop) return true;
    if (B.JoinedThreads.count(A.ThreadId) && !A.CreatedInLoop) return true;
    return false;
}

// Drugi dokaz: ako je A u "root" niti (main-stil) i u trenutku A još
// ne znamo za B - A sigurno dolazi pre nego što je B kreiran.
//
// Radi SAMO kad je jedna strana root - kod dve obicne (ne-root) niti
// bi ovo moglo pogresno da ih iskljuci, iako su nezavisne cim se kreiraju.
template <typename T>
bool HasCreationOrderPrecedence(const T &A, const T &B) {
    if (A.ThreadId == B.ThreadId) return false;

    bool AIsRoot = A.ThreadId.rfind("create_line_", 0) != 0;
    bool BIsRoot = B.ThreadId.rfind("create_line_", 0) != 0;

    if (AIsRoot && !A.KnownThreadsAtThisPoint.count(B.ThreadId)) return true;
    if (BIsRoot && !B.KnownThreadsAtThisPoint.count(A.ThreadId)) return true;
    return false;
}

// Kombinovana provera - true ako postoji BILO KOJI dokaz ne-konkurentnosti.
template <typename T>
bool IsDefinitelyExcluded(const T &A, const T &B) {
    return HasJoinPrecedence(A, B) || HasCreationOrderPrecedence(A, B);
}

#endif