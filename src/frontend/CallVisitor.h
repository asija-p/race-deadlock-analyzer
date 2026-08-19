#ifndef CALLVISITOR_H
#define CALLVISITOR_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>

using namespace clang;

class CallFinderVisitor : public RecursiveASTVisitor<CallFinderVisitor> {
public:
    explicit CallFinderVisitor(SourceManager &SM);

    bool VisitCallExpr(CallExpr *Call);

private:
    SourceManager &SM;
};

#endif