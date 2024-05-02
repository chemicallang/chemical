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

    FunctionCall(std::unique_ptr<Value> identifier, std::vector<std::unique_ptr<Value>> values);

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

    std::unique_ptr<BaseType> create_type() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::InvokeInst *llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind);

    llvm::Value * llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, std::vector<std::unique_ptr<Value>>& chain);

#endif

    std::string representation() const override;

    /**
     * the linked node
     * if a function call refers to a function declaration, it will return it
     * it can return to a pointer, passed through function param, so it can function param
     * a pointer could be stored in var init, so it could be var init
     */
    inline ASTNode* linked() {
        return name->linked_node();
    }

    /**
     * get linked node as a function
     * you should call this when you are sure, that this call is to a function
     * which is a function declaration
     */
    inline FunctionDeclaration* linked_func() {
        return name->linked_node()->as_function();
    }

    /**
     * if this call refers to a function declaration, returns it, otherwise not
     * so its safe
     */
    inline FunctionDeclaration* safe_linked_func() {
        return name->linked_node() ? name->linked_node()->as_function() : nullptr;
    }

    std::unique_ptr<Value> name;
    std::vector<std::unique_ptr<Value>> values;

};