#ifndef CFGPRINTER_H
#define CFGPRINTER_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

using namespace clang;

void PrintCFGForFunction(FunctionDecl *FD, ASTContext &Context);

#endif