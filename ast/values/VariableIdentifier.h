// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include "ast/base/ChainValue.h"
#include "ast/statements/VarInit.h"
#include "ast/utils/ExpressionEvaluator.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmfwd.h"

#endif

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public ChainValue {
public:

    /**
     * string value of identifier
     */
    std::string value;
    ASTNode *linked = nullptr;
    ChainValue* parent_val = nullptr;
    CSTToken* token;
    bool is_ns;
    bool is_moved = false;

    /**
     * constructor
     */
    VariableIdentifier(std::string value, CSTToken* token, bool is_ns = false) : value(std::move(value)), token(token), is_ns(is_ns) {

    }

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::Identifier;
    }

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    VariableIdentifier *as_identifier() override {
        return this;
    }

    BaseType* known_type() override;

    Value *child(InterpretScope &scope, const std::string &name) override;

    // will find value by this name in the parent
    Value *find_in(InterpretScope &scope, Value *parent) override;

    bool reference() override;

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) override;

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    bool link(SymbolResolver &linker, ChainValue*& value_ptr, bool prepend, bool check_access);

    bool link(SymbolResolver &linker, std::vector<ChainValue *> &values, unsigned int index, BaseType *expected_type) override {
        const auto values_size = values.size();
        const auto parent_index = index - 1;
        const auto parent = parent_index < values_size ? values[parent_index] : nullptr;
        if(parent) {
            return find_link_in_parent(parent, linker, expected_type);
        } else {
            return link(linker, values[index], false, false);
        }
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override {
        return link(linker, (ChainValue* &) (value_ptr), false, true);
    }

    bool link_assign(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        return link(linker, (ChainValue* &) (value_ptr), false, false);
    }

    void relink_parent(ChainValue *parent) override;

    ASTNode *linked_node() override;

    bool find_link_in_parent(ChainValue *parent, ASTDiagnoser *diagnoser);

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type);

    bool primitive() override {
        return false;
    }

    bool compile_time_computable() override;

#ifdef COMPILER_BUILD

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::Value *access_chain_value(Codegen &gen, std::vector<ChainValue*> &values, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) override;

#endif

    VarInitStatement *declaration() override;

    Value* evaluated_value(InterpretScope &scope) override;

//    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override;

    Value *evaluated_chain_value(InterpretScope &scope, Value *parent) override;
//    hybrid_ptr<Value> evaluated_chain_value(InterpretScope &scope, Value* parent) override;

    VariableIdentifier *copy(ASTAllocator& allocator) override;

    Value *scope_value(InterpretScope &scope) override;

    BaseType* create_type(ASTAllocator &allocator) override;
//    std::unique_ptr<BaseType> create_type() override;

//    hybrid_ptr<BaseType> get_base_type() override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

    [[nodiscard]]
    ValueType value_type() const override;

};