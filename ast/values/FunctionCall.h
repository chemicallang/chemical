// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/FunctionDeclaration.h"

class FunctionCall : public Value {
public:

    FunctionCall(std::string name, std::vector<std::unique_ptr<Value>> values);

    FunctionCall(FunctionCall &&other) = delete;

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void link(SymbolResolver &linker) override;

    FunctionCall *as_func_call() override;

    ASTNode *linked_node() override;

    ASTNode *find_link_in_parent(ASTNode *parent) override;

    bool primitive() override;

    Value *find_in(InterpretScope &scope, Value *parent) override;

    Value *evaluated_value(InterpretScope &scope) override;

    Value *copy() override;

    Value *initializer_value(InterpretScope &scope) override;

    Value *assignment_value(InterpretScope &scope) override;

    Value *param_value(InterpretScope &scope) override;

    Value *return_value(InterpretScope &scope) override;

    void interpret(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    std::string representation() const override;

    std::string name;
    std::vector<std::unique_ptr<Value>> values;
    ASTNode *linked = nullptr;

};