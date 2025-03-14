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
    std::vector<unsigned int> sizes;
    ArrayType* created_type;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst *arr;
#endif

    /**
     * constructor
     */
    constexpr ArrayValue(
            BaseType* elem_type,
            SourceLocation location,
            ASTAllocator& allocator
    ) : Value(ValueKind::ArrayValue, location) {
        created_type = new (allocator.allocate<ArrayType>()) ArrayType(elem_type, array_size(), ZERO_LOC);
    }

    /**
     * constructor
     */
    constexpr ArrayValue(
            std::vector<Value*> values,
            BaseType* elem_type,
            std::vector<unsigned int> sizes,
            SourceLocation location,
            ASTAllocator& allocator
    ) : Value(ValueKind::ArrayValue, location), values(std::move(values)), sizes(std::move(sizes)) {
        created_type = new (allocator.allocate<ArrayType>()) ArrayType(elem_type, array_size(), ZERO_LOC);
    }

    BaseType*& known_elem_type() const;

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

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    [[nodiscard]]
    BaseType* element_type(ASTAllocator& allocator) const;

    BaseType* create_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

    ArrayValue *copy(ASTAllocator& allocator) final {
        std::vector<Value*> copied_values;
        copied_values.reserve(values.size());
        for (const auto &value: values) {
            copied_values.emplace_back(value->copy(allocator));
        }
        std::vector<unsigned int> copied_sizes(sizes.size());
        BaseType* copied_elem_type = nullptr;
        const auto elemType = known_elem_type();
        if (elemType) {
            copied_elem_type = elemType->copy(allocator);
        }
        return new (allocator.allocate<ArrayValue>()) ArrayValue(std::move(copied_values), copied_elem_type, sizes, encoded_location(), allocator);
    }

};