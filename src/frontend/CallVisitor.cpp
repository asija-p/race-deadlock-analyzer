#include "CallVisitor.h"
#include <iostream>

CallFinderVisitor::CallFinderVisitor(SourceManager &SM) : SM(SM) {}

bool CallFinderVisitor::VisitCallExpr(CallExpr *Call) {
    FunctionDecl *Callee = Call->getDirectCallee();
    if (!Callee) {
        return true;
    }

    unsigned Line = SM.getSpellingLineNumber(Call->getBeginLoc());
    std::string FuncName = Callee->getNameAsString();

    std::string ArgName = "?";
    if (Call->getNumArgs() > 0) {
        Expr *Arg = Call->getArg(0);
        if (auto *Unary = dyn_cast<UnaryOperator>(Arg->IgnoreParenImpCasts())) {
            Arg = Unary->getSubExpr();
        }
        if (auto *Ref = dyn_cast<DeclRefExpr>(Arg->IgnoreParenImpCasts())) {
            ArgName = Ref->getDecl()->getNameAsString();
        }
    }

    std::cout << "Linija " << Line << ": " << FuncName
              << "(" << ArgName << ")\n";

    return true;
}