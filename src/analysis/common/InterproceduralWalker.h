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

// InterproceduralWalker sam radi:
//   - obilazak CFG-a sa spajanjem stanja na granama,
//   - ulazak u pozvane funkcije (i pamti sta je vec obradjeno),
//   - prati pthread_create/pthread_join (ko je posle koga).
//
// Visitor ne zna nista o tome - samo reaguje na ono sto je NJEMU vazno
// (npr. lock/unlock pozivi kod deadlocka, citanje/pisanje promenljivih
// kod race-a) i sam pamti svoj rezultat (Walker ne zna sta ce visitor
// da vrati - deadlock pravi listu LockPair, race listu MemoryAccess).
class AnalysisVisitor {
public:
    virtual ~AnalysisVisitor() = default;

    // Poziva se za SVAKI poziv funkcije u bloku, pre nego sto Walker
    // eventualno sam udje u tu funkciju. pthread_create/pthread_join
    // Walker uvek obradjuje sam, nikad ne stizu ovde.
    //
    // Vrati true = "ja sam ovo vec obradio, Walker nista vise ne radi"
    // (npr. lock/unlock pozivi).
    // Vrati false = pusti Walkeru da sam udje u funkciju (ako ima telo).
    virtual bool OnCallExpr(const CallExpr *Call, const FunctionDecl *Callee,
                             const std::string &FuncName, LockState &State,
                             const CFGBlock *Block,
                             const std::map<std::string, std::string> &ParamMap,
                             const std::string &ThreadId,
                             ASTContext &Context,
                             const std::set<std::string> &CreatedInLoop) = 0;


    virtual void OnStmt(const Stmt *S, LockState &State, const CFGBlock *Block,
                         const std::map<std::string, std::string> &ParamMap,
                         const std::string &ThreadId, ASTContext &Context,
                         const std::set<std::string> &CreatedInLoop) {}

    virtual void EnterNestedCall() {}
    virtual void ExitNestedCall() {}

    virtual void BeginBlock(const CFGBlock *Block) {}
    virtual void EndBlock(const CFGBlock *Block) {}

    virtual void FlushCFG() {}
};

LockState WalkFunction(const FunctionDecl *FD, ASTContext &Context,
                        LockState InitialState, AnalysisVisitor &Visitor,
                        CallStackMap &CallStack,
                        const std::map<std::string, std::string> &ParamMap,
                        const std::string &ThreadId,
                        std::set<std::string> &CreatedInLoop);

#endif
