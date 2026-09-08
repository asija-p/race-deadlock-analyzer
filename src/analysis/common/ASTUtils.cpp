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

bool IsSharedVariable(const ValueDecl *D) {
    if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (VD->hasGlobalStorage()) return true;   // globalna ili static
        // lokalna, automatic-storage - deljena SAMO ako joj je adresa
        // "pobegla" (prosledjena niti preko pthread_create argumenta) -
        // to je Deo 2, van obima ovog projekta.
        return false;
    }
    // Referenca na funkciju (npr. ime thread-funkcije prosledjeno kao
    // pthread_create argument) NIJE deljeni PODATAK - ne treba je pratiti
    // kao potencijalni race, cak ni konzervativno.
    if (isa<FunctionDecl>(D)) return false;
    return true;  
}

bool IsSharedAccess(const Expr *Arg) {
    Arg = Arg->IgnoreParenImpCasts();
    if (auto *Unary = dyn_cast<UnaryOperator>(Arg)) {
        // *ptr - ne znamo odakle pokazivac dolazi, konzervativno deljeno.
        if (Unary->getOpcode() == UO_Deref) return true;
        Arg = Unary->getSubExpr()->IgnoreParenImpCasts();
    }

    if (auto *ArrSub = dyn_cast<ArraySubscriptExpr>(Arg)) {
        // arr[i] - ako je BAZA prost lokalni niz (bez uzete adrese van
        // funkcije), ista logika kao za obicnu lokalnu promenljivu: ne
        // moze izazvati race. Ako je baza pokazivac/slozeniji izraz (npr.
        // p[i] gde je p parametar), ne pratimo poreklo - konzervativno
        // deljeno.
        const Expr *Base = ArrSub->getBase()->IgnoreParenImpCasts();
        if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
            return IsSharedVariable(BaseRef->getDecl());
        }
        return true;
    }

    if (auto *Member = dyn_cast<MemberExpr>(Arg)) {
        // obj.polje - ista logika po BAZI objekta (obj), ne po polju.
        // p->polje (dereferenciran pokazivac) ostaje konzervativno deljeno
        // - ne pratimo odakle pokazivac dolazi (Deo 2).
        const Expr *Base = Member->getBase()->IgnoreParenImpCasts();
        if (auto *DerefOp = dyn_cast<UnaryOperator>(Base)) {
            if (DerefOp->getOpcode() == UO_Deref) return true;
        }
        if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
            return IsSharedVariable(BaseRef->getDecl());
        }
        return true;
    }

    if (auto *Ref = dyn_cast<DeclRefExpr>(Arg)) {
        return IsSharedVariable(Ref->getDecl());
    }

    return true;   // nepoznat oblik - konzervativno deljeno
}