#ifndef INTERPROCEDURALWALKER_H
#define INTERPROCEDURALWALKER_H

#include "LockState.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Analysis/CFG.h>
#include <map>
#include <set>
#include <string>

using namespace clang;

// Pamti stanje sa kojim je funkcija VEC uladjena, da bi se izbeglo
// beskonacno/redundantno ponovno ulazenje (rekurzija ili isti poziv sa
// istim kontekstom sa vise mesta u kodu).
struct CallContext {
    LockState State;
    std::map<std::string, std::string> ParamMap;

    bool operator==(const CallContext &Other) const {
        return State == Other.State && ParamMap == Other.ParamMap;
    }
    bool operator!=(const CallContext &Other) const {
        return !(*this == Other);
    }
};

using CallStackMap = std::map<const FunctionDecl *, CallContext>;

// Implementira ga svaka konkretna analiza (deadlock, race, ...).
//
// InterproceduralWalker sam resava:
//   - CFG worklist/fixpoint obilazak sa merge-om stanja na spoju grana,
//   - interproceduralnu rekurziju sa call-context memoizacijom,
//   - pthread_create/pthread_join happens-before knjigovodstvo.
//
// Visitor NE zna nista o tome - reaguje samo na ono sto je specificno za
// njegov domen (npr. lock/unlock pozivi kod deadlocka, citanja/pisanja
// deljenih promenljivih kod race analize) i sam vodi racuna o svom
// rezultatu (Walker o tipu rezultata nista ne zna - deadlock generise
// std::vector<LockPair>, race bi generisao npr. std::vector<MemoryAccess>).
class AnalysisVisitor {
public:
    virtual ~AnalysisVisitor() = default;

    // Pozvano za SVAKI CallExpr u bloku, PRE nego sto Walker eventualno
    // uradi genericki interproceduralni ulazak u telo pozvane funkcije.
    // pthread_create/pthread_join Walker obradjuje sam i NIKAD ih ne
    // prosledjuje ovde.
    //
    // Vratiti true = "ja sam ovo obradio, Walker dalje NISTA ne radi za
    // ovaj poziv" (npr. lock/unlock - spoljna funkcija bez tela, tako da
    // generic interproceduralni ulazak ionako ne bi imao efekta, ali je
    // korektnije eksplicitno reci da je "potroseno").
    // Vratiti false = pusti Walkeru da, AKO Callee ima definiciju sa telom,
    // uradi standardni interproceduralni ulazak (mapiranje parametara,
    // memoizacija, rekurzija).
    virtual bool OnCallExpr(const CallExpr *Call, const FunctionDecl *Callee,
                             const std::string &FuncName, LockState &State,
                             const CFGBlock *Block,
                             const std::map<std::string, std::string> &ParamMap,
                             const std::string &ThreadId,
                             ASTContext &Context,
                             const std::set<std::string> &CreatedInLoop) = 0;

    // Pozvano za SVAKI Stmt u bloku (ukljucujuci i pozive, PRE OnCallExpr
    // kuke za njih). Podrazumevano ne radi nista - deadlock analiza ovo ne
    // koristi. Koristi ga analiza kojoj trebaju i ne-poziv iskazi, npr.
    // race analiza koja gleda dodele/citanja promenljivih.
    virtual void OnStmt(const Stmt *S, LockState &State, const CFGBlock *Block,
                         const std::map<std::string, std::string> &ParamMap,
                         const std::string &ThreadId, ASTContext &Context,
                         const std::set<std::string> &CreatedInLoop) {}

    // Pozvano tacno jednom pre obrade prvog iskaza u bloku, odnosno tacno
    // jednom posle obrade poslednjeg. Ako fixpoint algoritam ponovo obradi
    // isti blok (jer mu se ulazno stanje promenilo), ovaj par se opet
    // pozove - analiza koja generise rezultate PO BLOKU (kao deadlock)
    // treba u BeginBlock da odbaci rezultate iz prethodne obrade istog
    // bloka, da fixpoint ponavljanje ne bi ostavilo zastarele/duplirane
    // zapise u konacnom rezultatu.
    virtual void EnterNestedCall() {}
    virtual void ExitNestedCall() {}

    virtual void BeginBlock(const CFGBlock *Block) {}
    virtual void EndBlock(const CFGBlock *Block) {}

    // Pozvano TACNO JEDNOM po svakom WalkFunction pozivu, odmah nakon sto
    // fixpoint za tu KONKRETNU CFG analizu zavrsi. Analiza koja bafferuje
    // rezultate PO BLOKU (npr. deadlock) MORA ovde da prebaci svoj bafer u
    // Result i da ga OCISTI - CFGBlock* adrese vaze samo dok zivi TA
    // KONKRETNA CFG, pa odlaganje flush-a do kraja cele rekurzivne analize
    // dovodi do kolizije adresa izmedju razlicitih funkcija.
    virtual void FlushCFG() {}
};

// Pokrece interproceduralnu, worklist/fixpoint analizu tela funkcije FD,
// pozivajuci Visitor kukice usput. Vraca LockState na izlazu iz funkcije
// (na Exit CFG bloku).
LockState WalkFunction(const FunctionDecl *FD, ASTContext &Context,
                        LockState InitialState, AnalysisVisitor &Visitor,
                        CallStackMap &CallStack,
                        const std::map<std::string, std::string> &ParamMap,
                        const std::string &ThreadId,
                        std::set<std::string> &CreatedInLoop);

#endif
