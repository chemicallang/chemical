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
            std::string structName,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            StructDefinition *definition = nullptr
    );

    StructValue(
            std::string structName,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            StructDefinition *definition,
            InterpretScope &scope
    );

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    bool primitive() override;

    void link(SymbolResolver &linker) override;

    Value *call_member(
            InterpretScope &scope,
            const std::string &name,
            std::vector<std::unique_ptr<Value>> &params
    ) override;

    void set_child_value(const std::string &name, Value *value, Operation op) override;

    Value *evaluated_value(InterpretScope &scope) override;

    Value *initializer_value(InterpretScope &scope) override;

    void declare_default_values(std::unordered_map<std::string, std::unique_ptr<Value>> &into, InterpretScope &scope);

    Value *copy() override;

    Value *child(InterpretScope &scope, const std::string &name) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::AllocaInst *llvm_allocate(Codegen &gen, const std::string &identifier) override;

    unsigned int store_in_struct(
            Codegen &gen,
            StructValue *parent,
            llvm::AllocaInst *ptr,
            std::vector<llvm::Value*> idxList,
            const std::string &identifier,
            unsigned int index
    ) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_indexes(Codegen &gen, std::vector<llvm::Value *> &indexes, std::vector<std::unique_ptr<Value>> &u_inds) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::string representation() const override;

    StructValue *as_struct() override;

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    std::string structName;
    StructDefinition *definition = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Value>> values;

};