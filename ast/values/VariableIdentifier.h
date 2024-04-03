// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/Value.h"
#include "ast/statements/VarInit.h"
#include "ast/utils/ExpressionEvaluator.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmfwd.h"

#endif

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public Value {
public:

    /**
     * @brief Construct a new VariableIdentifier object.
     *
     * @param value The string value.
     */
    VariableIdentifier(std::string value) : value(std::move(value)) {}

    Value *child(InterpretScope &scope, const std::string &name) override;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) override;

    bool reference() override;

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) override;

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    ASTNode *find_link_in_parent(ASTNode *parent) override;

    bool add_member_index(Codegen &gen, ASTNode *parent, std::vector<llvm::Value *> &indexes) override;

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    VarInitStatement *declaration() override;

    Value *evaluated_value(InterpretScope &scope) override;

    /**
     * every identifier's value will be moved to new owner at return
     */
    Value *return_value(InterpretScope &scope) override;

    /**
     * copy the value if its primitive, otherwise make a reference
     */
    Value *copy_prim_ref_other(InterpretScope &scope);

    Value *param_value(InterpretScope &scope) override;

    Value *initializer_value(InterpretScope &scope) override;

    Value *assignment_value(InterpretScope &scope) override;

    std::string representation() const override;

private:
    ASTNode *linked = nullptr;
    std::string value; ///< The string value.

};