// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * unsafe values replace themselves during symbol resolution
 * TODO: not yet ready
 */
class UnsafeValue : public Value {
private:

    /**
     * the actual value
     */
    Value* value;

public:

    /**
     * the allocator used at the time of creation
     */
    ASTAllocator* allocator;

    /**
     * constructor
     */
    inline constexpr UnsafeValue(
        ASTAllocator* allocator,
        Value* value
    ) : Value(ValueKind::UnsafeValue, value->getType(), value->encoded_location()), allocator(allocator), value(value) {

    }

    inline Value* getValue() {
        return value;
    }

    void setValue(Value* newValue) {
        value = newValue;
        setType(newValue->getType());
    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<UnsafeValue>()) UnsafeValue(
            &allocator, value->copy(allocator)
        );
    }

#ifdef COMPILER_BUILD

    // `unsafe(expr)` is a compile-time-only safety marker. It has no runtime
    // effect, so every code-generation entry point simply forwards to the
    // wrapped value (mirroring the C-backend's `VisitUnsafeValue`).

    llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr) final {
        return value->llvm_value(gen, type);
    }

    llvm::Value* llvm_pointer(Codegen& gen) final {
        return value->llvm_pointer(gen);
    }

    llvm::Type* llvm_type(Codegen& gen) final {
        return value->llvm_type(gen);
    }

    llvm::Type* llvm_chain_type(Codegen& gen, std::vector<Value*>& values, unsigned int index) final {
        return value->llvm_chain_type(gen, values, index);
    }

    llvm::Value* llvm_arg_value(Codegen& gen, BaseType* expected_type) final {
        return value->llvm_arg_value(gen, expected_type);
    }

    llvm::Value* llvm_ret_value(Codegen& gen, Value* returnValue) final {
        return value->llvm_ret_value(gen, returnValue);
    }

    void llvm_assign_value(Codegen& gen, llvm::Value* storagePtr, Value* lhs, llvm::Value* lhsPtr) final {
        value->llvm_assign_value(gen, storagePtr, lhs, lhsPtr);
    }

    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) final {
        value->llvm_conditional_branch(gen, then_block, otherwise_block);
    }

    llvm::AllocaInst* llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final {
        return value->llvm_allocate(gen, identifier, expected_type);
    }

    bool add_member_index(Codegen& gen, Value* parent, std::vector<llvm::Value*>& indexes) final {
        return value->add_member_index(gen, parent, indexes);
    }

    bool add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const chem::string_view& name) final {
        return value->add_child_index(gen, indexes, name);
    }

    llvm::AllocaInst* access_chain_allocate(
        Codegen& gen,
        std::vector<Value*>& values,
        unsigned int until,
        BaseType* expected_type
    ) final {
        return value->access_chain_allocate(gen, values, until, expected_type);
    }

#endif

};