// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include <string>
#include "preprocess/visitors/RecursiveVisitor.h"

class CAfterStmtVisitor : public RecursiveVisitor<CAfterStmtVisitor>, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    bool destruct_call = false;

    void destruct_chain(AccessChain *chain, bool destruct_last);

    void VisitFunctionCall(FunctionCall *call);

    void VisitAccessChain(AccessChain *chain);

    void VisitLambdaFunction(LambdaFunction *func) {
        // do nothing
    }

    void VisitScope(Scope *scope) {
        // do nothing
    }

};