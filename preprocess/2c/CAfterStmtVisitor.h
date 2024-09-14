// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CAfterStmtVisitor : public CommonVisitor, public SubVisitor {

    using SubVisitor::SubVisitor;

    void visit(VariableIdentifier *identifier) override;

    void visit(FunctionCall *call) override;

    void visit(AccessChain *chain) override;

    void destruct_chain(AccessChain *chain, bool destruct_last);

    void visit(LambdaFunction *func) override {
        // do nothing
    }

    void visit(Scope *scope) override {
        // do nothing
    }

};