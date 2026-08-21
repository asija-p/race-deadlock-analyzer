#ifndef LOCKORDERANALYZER_H
#define LOCKORDERANALYZER_H

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <vector>
#include <string>
#include <utility>

using namespace clang;

// Par (A, B) znaci: mutex A je zakljucan pre mutexa B, na nekoj putanji
using LockPair = std::pair<std::string, std::string>;

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context);

#endif