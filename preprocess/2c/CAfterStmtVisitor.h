// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include <string>
#include "preprocess/visitors/RecursiveValueVisitor.h"

class CAfterStmtVisitor : public RecursiveValueVisitor<CAfterStmtVisitor>, public SubVisitor {
public:

    using RecursiveValueVisitor<CAfterStmtVisitor>::visit;
    using SubVisitor::SubVisitor;

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