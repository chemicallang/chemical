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

    Value* evaluated_value(InterpretScope& scope) override {
        if(!value) return this;
        auto evaluated = value->evaluated_value(scope);
        if(evaluated && evaluated != value) {
            return evaluated;
        }
        return this;
    }

    Value* child(InterpretScope& scope, const chem::string_view& name) override {
        if(!value) return nullptr;
        // Evaluate the inner value to get the actual struct, then delegate child lookup
        auto inner = value->evaluated_value(scope);
        if(!inner || inner == value) {
            inner = this;
        }
        return inner->child(scope, name);
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

    void llvm_assign_value(Codegen &gen, llvm::Value *storagePtr, Value *lhs, llvm::Value *lhsPtr) override;

    unsigned int store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    unsigned int store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

#endif

};