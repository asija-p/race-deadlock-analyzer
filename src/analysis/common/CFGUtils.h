#ifndef CFGUTILS_H
#define CFGUTILS_H

#include <clang/Analysis/CFG.h>

using namespace clang;

// Da li je Start deo ciklusa u CFG grafu (tj. da li postoji put napred od
// njegovih naslednika koji se vraca nazad na Start). Cista dostiznost na
// statickoj strukturi grafa - NE zavisi od worklist fixpoint obrade, pa ne
// pravi lazne pozitive na obicnom if/else grananju. Domenski neutralno -
// koristi ga svaka analiza kojoj treba "da li je ovaj poziv/pristup unutar
// petlje" (npr. da oznaci da isti ThreadId moze predstavljati vise niti).
bool IsBlockInLoop(const CFGBlock *Start);

#endif
