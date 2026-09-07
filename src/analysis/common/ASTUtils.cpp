#include "ASTUtils.h"

std::string ExtractVarName(const Expr *Arg) {
    Arg = Arg->IgnoreParenImpCasts();
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
            return ArrayName + "[" + std::to_string(IntLit->getValue().getSExtValue()) + "]";
        }
        return ArrayName + "[?]";
    }

    if (auto *Member = dyn_cast<MemberExpr>(Arg)) {
        std::string BaseName = "?";
        const Expr *Base = Member->getBase()->IgnoreParenImpCasts();
        if (auto *DerefOp = dyn_cast<UnaryOperator>(Base)) {
            if (DerefOp->getOpcode() == UO_Deref) {
                Base = DerefOp->getSubExpr()->IgnoreParenImpCasts();
            }
        }
        if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
            BaseName = BaseRef->getDecl()->getNameAsString();
        }
        std::string FieldName = Member->getMemberDecl()->getNameAsString();
        return BaseName + "." + FieldName;
    }

    if (auto *Ref = dyn_cast<DeclRefExpr>(Arg)) {
        return Ref->getDecl()->getNameAsString();
    }
    return "?";
}

std::string ResolveName(const std::string &Name,
                         const std::map<std::string, std::string> &ParamMap) {
    auto It = ParamMap.find(Name);
    if (It != ParamMap.end()) {
        return It->second;
    }
    return Name;
}
