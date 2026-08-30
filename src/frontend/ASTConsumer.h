// Globalni flag - kada je true, HandleTranslationUnit ne ispisuje
// CFG dump ni "lep" tekst, samo kratak rezultat za automatsko testiranje.
extern bool QuietMode;

#ifndef ASTCONSUMER_H
#define ASTCONSUMER_H

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <iostream>

using namespace clang;

class DumpASTConsumer : public ASTConsumer {
public:
    void HandleTranslationUnit(ASTContext &Context) override;
};

class DumpASTAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI, StringRef file) override;
};

#endif