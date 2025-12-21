// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include "preprocess/visitors/RecursiveVisitor.h"
#include <string>
#include <unordered_map>

class CValueDeclarationVisitor : public RecursiveVisitor<CValueDeclarationVisitor>, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    std::unordered_map<void*, std::string> aliases;

    unsigned lambda_num = 0;

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitFunctionDecl(FunctionDeclaration *functionDeclaration);

    void VisitFunctionType(FunctionType *func);

    void VisitStructMember(StructMember *member);

    void VisitIfStmt(IfStatement *ifStatement);

    void reset() final {
        aliases.clear();
    }

};