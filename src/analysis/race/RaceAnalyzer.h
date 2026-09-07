#ifndef RACEANALYZER_H
#define RACEANALYZER_H

#include "MemoryAccess.h"
#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <set>
#include <string>
#include <vector>

using namespace clang;

std::vector<MemoryAccess> FindMemoryAccesses(FunctionDecl *FD, ASTContext &Context,
                                              std::set<std::string> &CreatedInLoop);

#endif
