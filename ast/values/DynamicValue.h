// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/DynamicType.h"

/**
 * the dynamic value is created when you write dyn<Type>(expr)
 */
class DynamicValue : public Value {
public:

    Value* value;

    /**
     * constructor
     */
    DynamicValue(
            Value* value,
            SourceLocation location,
            DynamicType* dynType
    ) : Value(ValueKind::DynamicValue, dynType, location), value(value) {

    }

    DynamicType* getType() const noexcept {
        return (DynamicType*) Value::getType();
    }

    [[nodiscard]]
    BaseType* getInterfaceType() const noexcept {
        return getType()->referenced;
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto dyn_type = new (allocator.allocate<DynamicType>()) DynamicType(
            getType()->referenced->copy(allocator)
        );
        const auto dyn_val = new (allocator.allocate<DynamicValue>()) DynamicValue(
            value->copy(allocator), encoded_location(), dyn_type
        );
        return dyn_val;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

    llvm::Value* llvm_ret_value(Codegen &gen, Value *returnValue) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    unsigned int store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    unsigned int store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

#endif

};