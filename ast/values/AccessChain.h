// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class AccessChain : public ASTNode, public Value {

public:

    AccessChain(std::vector<std::unique_ptr<Value>> values);

    void link(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor &visitor) override;

    bool primitive() override;

    bool reference() override;

    void interpret(InterpretScope &scope) override;

    std::unique_ptr<BaseType> create_type() const override;

    std::unique_ptr<BaseType> create_value_type() override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;
#endif

    Value *parent(InterpretScope &scope);

    inline Value *parent_value(InterpretScope &scope);

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    std::string interpret_representation() const override;

    Value *pointer(InterpretScope &scope);

    Value *evaluated_value(InterpretScope &scope) override;

    Value *param_value(InterpretScope &scope) override;

    Value *initializer_value(InterpretScope &scope) override;

    Value *assignment_value(InterpretScope &scope) override;

    Value *return_value(InterpretScope &scope) override;

    std::string representation() const override;

    std::vector<std::unique_ptr<Value>> values;

};