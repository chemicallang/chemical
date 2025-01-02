// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CBeforeStmtVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    void visit(FunctionCall *call) final;

    void visit(AccessChain *chain) final;

    void visit(VariantCall *call) final;

    void process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const chem::string_view& identifier);

    void process_init_value(Value* value, const chem::string_view& identifier);

    void visit(VariableIdentifier *identifier) final;

    void visit(VarInitStatement *init) final;

    void visit(Scope *scope) final {
        // do nothing
    }

    void visit(LambdaFunction *func) final {
        // do nothing
    }

};