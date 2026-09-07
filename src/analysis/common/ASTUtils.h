#ifndef ASTUTILS_H
#define ASTUTILS_H

#include <clang/AST/Expr.h>
#include <map>
#include <string>

using namespace clang;

// Izvlaci citljivo ime iz izraza koji predstavlja argument poziva ili
// operand u iskazu: prosta promenljiva, arr[i], obj.polje, *ptr, itd.
// Koristi ga i deadlock (ime brave) i race (ime deljene promenljive) analiza.
std::string ExtractVarName(const Expr *Arg);

// Prevodi ime parametra u ime stvarnog argumenta na mestu poziva, koristeci
// ParamMap sagradjen pri ulasku u pozvanu funkciju (interproceduralno
// "supstituisanje" imena).
std::string ResolveName(const std::string &Name,
                         const std::map<std::string, std::string> &ParamMap);

#endif
