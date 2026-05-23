// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include "preprocess/visitors/RecursiveVisitor.h"
#include <string>

class CBeforeStmtVisitor : public RecursiveVisitor<CBeforeStmtVisitor>, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    bool destruct_call = false;

    void VisitFunctionCall(FunctionCall *call);

    void VisitVariableIdentifier(VariableIdentifier *identifier) {

    }

    void VisitScope(Scope *scope) {
        // do nothing
    }

    void VisitLambdaFunction(LambdaFunction *func) {
        // do nothing
    }

};