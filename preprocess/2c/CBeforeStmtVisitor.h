// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CBeforeStmtVisitor : public CommonVisitor, public SubVisitor {
public:

    using SubVisitor::SubVisitor;

    void visit(FunctionCall *call) override;

    void visit(AccessChain *chain) override;

    void visit(VariantCall *call) override;

    void process_comp_time_call(FunctionDeclaration* decl, FunctionCall* call, const std::string& identifier);

    void process_init_value(Value* value, const std::string& identifier);

    void visit(VarInitStatement *init) override;

    void visit(Scope *scope) override {
        // do nothing
    }

    void visit(LambdaFunction *func) override {
        // do nothing
    }

};