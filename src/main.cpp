#include <iostream>
#include <fstream>
#include <sstream>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;
using namespace clang::tooling;

class CallFinderVisitor : public RecursiveASTVisitor<CallFinderVisitor> {
public:
    // Konstruktor - prima SourceManager i pamti ga za kasnije koriscenje
    explicit CallFinderVisitor(SourceManager &SM) : SM(SM) {}

    bool VisitCallExpr(CallExpr *Call) {
        FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) {
            return true;
        }

        unsigned Line = SM.getSpellingLineNumber(Call->getBeginLoc());
        std::string FuncName = Callee->getNameAsString();

        std::string ArgName = "?";
        if (Call->getNumArgs() > 0) {
            Expr *Arg = Call->getArg(0);
            // Skidamo eventualni '&' (UnaryOperator) da dodjemo do prave promenljive
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

private:
    SourceManager &SM;
};

class DumpASTConsumer : public ASTConsumer {
public:
    void HandleTranslationUnit(ASTContext &Context) override {
        SourceManager &SM = Context.getSourceManager();
        TranslationUnitDecl *TU = Context.getTranslationUnitDecl();

        CallFinderVisitor Visitor(SM);

        for (Decl *D : TU->decls()) {
            //preskacemo sve sto nije iz naseg fajla
            if(!SM.isInMainFile(D->getLocation())) {
                continue;
            }
            // Zanimaju nas samo funkcije - lock1/lock2 (VarDecl) preskacemo
            if (auto *FD = dyn_cast<FunctionDecl>(D)) {
                // Preskacemo deklaracije bez tela (npr. "void foo();" bez definicije)
                if (!FD->hasBody()) {
                    continue;
                }
                std::cout << "\n--- Funkcija: " << FD->getNameAsString() << " ---\n";
                Visitor.TraverseDecl(FD);
            }
        }
    }
};

class DumpASTAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI, StringRef file) override {
        return std::make_unique<DumpASTConsumer>();
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Upotreba: " << argv[0] << " <putanja_do_c_fajla>\n";
        return 1;
    }

    std::ifstream inFile(argv[1]);
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string code = buffer.str();

    //std::cout << "Ucitan fajl, duzina: " << code.size() << " karaktera\n";
    //std::cout << "Sadrzaj:\n" << code << "\n";

    std::vector<std::string> args = {"-x", "c"};
    bool success = runToolOnCodeWithArgs(
        std::make_unique<DumpASTAction>(), code, args, argv[1]);

    return success ? 0 : 1;
}