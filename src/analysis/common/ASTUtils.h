#ifndef ASTUTILS_H
#define ASTUTILS_H

#include <clang/AST/Expr.h>
#include <map>
#include <string>

using namespace clang;

std::string ExtractVarName(const Expr *Arg);

std::string ResolveName(const std::string &Name,
                         const std::map<std::string, std::string> &ParamMap);

bool IsSharedVariable(const ValueDecl *D);

bool IsSharedAccess(const Expr *Arg);

#endif