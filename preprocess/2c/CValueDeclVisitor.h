// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include "preprocess/visitors/RecursiveValueVisitor.h"
#include <string>
#include <unordered_map>

class CValueDeclarationVisitor : public RecursiveValueVisitor<CValueDeclarationVisitor>, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    using RecursiveValueVisitor<CValueDeclarationVisitor>::visit;

    std::unordered_map<void*, std::string> aliases;

    unsigned lambda_num = 0;

    unsigned func_type_num = 0;

    unsigned alias_num = 0;

    unsigned enum_num = 0;

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitFunctionDecl(FunctionDeclaration *functionDeclaration);

    void VisitEnumDecl(EnumDeclaration *enumDeclaration);

    void VisitFunctionType(FunctionType *func);

    void VisitStructMember(StructMember *member);

    void VisitIfStmt(IfStatement *ifStatement);

    void VisitReturnStmt(ReturnStatement *stmt);

    void VisitFunctionCall(FunctionCall *call);

    void VisitArrayValue(ArrayValue *arrayVal);

    void VisitStructValue(StructValue *structValue);

    void reset() final {
        aliases.clear();
    }

};