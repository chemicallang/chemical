// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include "preprocess/visitors/RecursiveValueVisitor.h"
#include <string>

class CBeforeStmtVisitor : public RecursiveValueVisitor<CBeforeStmtVisitor>, public SubVisitor {
public:

    using RecursiveValueVisitor<CBeforeStmtVisitor>::visit;
    using SubVisitor::SubVisitor;

    void process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const chem::string_view& identifier);

    void process_init_value(Value* value, const chem::string_view& identifier);

    void VisitFunctionCall(FunctionCall *call);

    void VisitAccessChain(AccessChain *chain);

    void VisitVariableIdentifier(VariableIdentifier *identifier) ;

    void VisitVarInitStmt(VarInitStatement *init) ;

    void VisitScope(Scope *scope) {
        // do nothing
    }

    void VisitLambdaFunction(LambdaFunction *func) {
        // do nothing
    }

};