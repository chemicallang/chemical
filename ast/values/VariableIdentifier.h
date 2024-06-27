// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
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
     * string value of identifier
     */
    std::string value;
    ASTNode *linked = nullptr;

    /**
     * @brief Construct a new VariableIdentifier object.
     *
     * @param value The string value.
     */
    VariableIdentifier(std::string value) : value(std::move(value)) {}

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    VariableIdentifier *as_identifier() override {
        return this;
    }

    Value *child(InterpretScope &scope, const std::string &name) override;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) override;

    bool reference() override;

    virtual bool can_link_with_namespace() {
        return false;
    };

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) override;

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    void prepend_self(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr, BaseFunctionParam* self_param);

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    ASTNode *linked_node() override;

    void find_link_in_parent(Value *parent, SymbolResolver &resolver) override;

    bool primitive() override {
        return false;
    }

#ifdef COMPILER_BUILD

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::Value *access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned until) override;

#endif

    VarInitStatement *declaration() override;

    Value *evaluated_value(InterpretScope &scope) override;

    Value *copy() override;

    /**
     * every identifier's value will be moved to new owner at return
     */
    Value *return_value(InterpretScope &scope) override;

    /**
     * copy the value if its primitive, otherwise make a reference
     */
    Value *copy_prim_ref_other(InterpretScope &scope);

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    Value *param_value(InterpretScope &scope) override;

    Value *initializer_value(InterpretScope &scope) override;

    Value *assignment_value(InterpretScope &scope) override;

    BaseTypeKind type_kind() const override;

    ValueType value_type() const override;

};