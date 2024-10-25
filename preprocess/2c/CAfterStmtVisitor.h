// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CAfterStmtVisitor : public CommonVisitor, public SubVisitor {

    using SubVisitor::SubVisitor;

    void visit(FunctionCall *call) final;

    void visit(AccessChain *chain) final;

    void destruct_chain(AccessChain *chain, bool destruct_last);

    void visit(LambdaFunction *func) final {
        // do nothing
    }

    void visit(Scope *scope) final {
        // do nothing
    }

};