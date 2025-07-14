// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/types/ArrayType.h"

class ArrayValue : public Value {
public:

    std::vector<Value*> values;
    // TODO: remove sizes from here
    std::vector<unsigned int> sizes;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst *arr;
#endif

    /**
     * constructor
     */
    ArrayValue(
            TypeLoc elem_type,
            SourceLocation location,
            ASTAllocator& allocator
    ) : Value(
            ValueKind::ArrayValue,
            new (allocator.allocate<ArrayType>()) ArrayType(elem_type, static_cast<uint64_t>(0)),
            location
    ) {
    }

    /**
     * constructor
     */
    ArrayValue(
            SourceLocation location,
            ArrayType* type
    ) : Value(
            ValueKind::ArrayValue,
            type,
            location
    ) {
    }

    /**
     * get the type of this array value
     */
    inline ArrayType* getType() const noexcept {
        return (ArrayType*) Value::getType();
    }

    TypeLoc& known_elem_type() const;

    bool primitive() final {
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

    inline bool has_explicit_size() {
        return !sizes.empty();
    }

    inline void set_array_size(unsigned int siz) {
        if(sizes.empty()) {
            sizes.emplace_back(siz);
        } else {
            sizes[0] = siz;
        }
        getType()->set_array_size(siz);
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) final;

    void initialize_allocated(Codegen& gen, llvm::Value*, BaseType* expected_type);

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value *llvm_arg_value(Codegen &gen, BaseType* expected_type) final;

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) final;

    unsigned int store_in_struct(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) final;

    llvm::Type *llvm_elem_type(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    ASTNode *linked_node() final;

    [[nodiscard]]
    BaseType* element_type(ASTAllocator& allocator) const;

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    ArrayValue *copy(ASTAllocator& allocator) final {
        const auto arrVal = new (allocator.allocate<ArrayValue>()) ArrayValue(encoded_location(), (ArrayType*) getType()->copy(allocator));
        auto& copied_values = arrVal->values;
        copied_values.reserve(values.size());
        for (const auto &value: values) {
            copied_values.emplace_back(value->copy(allocator));
        }
        arrVal->sizes = sizes;
        return arrVal;
    }

};