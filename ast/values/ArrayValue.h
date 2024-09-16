// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/values/TypeLinkedValue.h"
#include "ast/types/ArrayType.h"

class ArrayValue : public Value, public TypeLinkedValue {
public:

    std::vector<std::unique_ptr<Value>> values;
    std::unique_ptr<BaseType> elemType;
    std::vector<unsigned int> sizes;
    CSTToken* token;
    std::unique_ptr<BaseType> cached_type = nullptr;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst *arr;
#endif

    ArrayValue(
            std::vector<std::unique_ptr<Value>> values,
            std::unique_ptr<BaseType> type,
            std::vector<unsigned int> sizes,
            CSTToken* token
    ) : values(std::move(values)), elemType(std::move(type)), sizes(std::move(sizes)), token(token) {
        values.shrink_to_fit();
    }

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::ArrayValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool primitive() override {
        return false;
    }

    [[nodiscard]]
    inline unsigned int array_size() const {
        if (sizes.empty()) {
            return values.size();
        } else {
            return sizes[0];
        }
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    void initialize_allocated(Codegen& gen, llvm::Value*, BaseType* expected_type);

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Value *llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) override;

    unsigned int store_in_struct(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    ASTNode *linked_node() override;

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *expected_type = nullptr) override;

    bool link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override {
        return TypeLinkedValue::link(linker, call, index);
    }

    bool link(SymbolResolver &linker, ReturnStatement *returnStmt) override {
        return TypeLinkedValue::link(linker, returnStmt);
    }

    [[nodiscard]]
    std::unique_ptr<BaseType> element_type() const;

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    BaseType* known_child_type();

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Array;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Array;
    }

    ArrayValue *copy() override {
        std::vector<std::unique_ptr<Value>> copied_values;
        copied_values.reserve(values.size());
        for (const auto &value: values) {
            copied_values.emplace_back(value->copy());
        }
        std::vector<unsigned int> copied_sizes(sizes.size());
        std::unique_ptr<BaseType> copied_elem_type = nullptr;
        if (elemType) {
            copied_elem_type.reset(elemType->copy());
        }
        return new ArrayValue(std::move(copied_values), std::move(copied_elem_type), sizes, token);
    }

};