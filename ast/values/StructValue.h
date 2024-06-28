// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"

class StructValue : public Value {
public:

    StructValue(
            std::unique_ptr<Value> ref,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            StructDefinition *definition = nullptr
    );

    StructValue(
            std::unique_ptr<Value> ref,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            StructDefinition *definition,
            InterpretScope &scope
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool primitive() override;

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    Value *call_member(
            InterpretScope &scope,
            const std::string &name,
            std::vector<std::unique_ptr<Value>> &params
    ) override;

    void set_child_value(const std::string &name, Value *value, Operation op) override;

    Value *scope_value(InterpretScope &scope) override;

    void declare_default_values(std::unordered_map<std::string, std::unique_ptr<Value>> &into, InterpretScope &scope);

    Value *copy() override;

    Value *child(InterpretScope &scope, const std::string &name) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    void initialize_alloca(llvm::Value *inst, Codegen& gen);

    llvm::AllocaInst *llvm_allocate(Codegen &gen, const std::string &identifier) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    unsigned int store_in_struct(
            Codegen &gen,
            StructValue *parent,
            llvm::Value *allocated,
            std::vector<llvm::Value *> idxList,
            unsigned int index
    ) override;

    unsigned int store_in_array(
            Codegen &gen,
            ArrayValue *parent,
            llvm::AllocaInst *ptr,
            std::vector<llvm::Value *> idxList,
            unsigned int index
    ) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::unique_ptr<BaseType> create_type() override;

    uint64_t byte_size(bool is64Bit) override;

    hybrid_ptr<BaseType> get_base_type() override;

    StructValue *as_struct() override;

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    std::unique_ptr<Value> ref;
    StructDefinition *definition = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Value>> values;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst;
#endif

};