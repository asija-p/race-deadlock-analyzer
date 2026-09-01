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
        Expr *Arg = Call->getArg(0)->IgnoreParenImpCasts();
        if (auto *Unary = dyn_cast<UnaryOperator>(Arg)) {
            Arg = Unary->getSubExpr()->IgnoreParenImpCasts();
        }

        if (auto *ArrSub = dyn_cast<ArraySubscriptExpr>(Arg)) {
            std::string ArrayName = "?";
            const Expr *Base = ArrSub->getBase()->IgnoreParenImpCasts();
            if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
                ArrayName = BaseRef->getDecl()->getNameAsString();
            }
            const Expr *IndexExpr = ArrSub->getIdx()->IgnoreParenImpCasts();
            if (auto *IntLit = dyn_cast<IntegerLiteral>(IndexExpr)) {
                ArgName = ArrayName + "[" + std::to_string(IntLit->getValue().getSExtValue()) + "]";
            } else {
                ArgName = ArrayName + "[?]";
            }
        } else if (auto *Member = dyn_cast<MemberExpr>(Arg)) {
            std::string BaseName = "?";
            const Expr *Base = Member->getBase()->IgnoreParenImpCasts();
            if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
                BaseName = BaseRef->getDecl()->getNameAsString();
            }
            std::string FieldName = Member->getMemberDecl()->getNameAsString();
            ArgName = BaseName + "." + FieldName;
        } else if (auto *Ref = dyn_cast<DeclRefExpr>(Arg)) {
            ArgName = Ref->getDecl()->getNameAsString();
        }
    }

    std::cout << "Linija " << Line << ": " << FuncName
              << "(" << ArgName << ")\n";

    return true;
}