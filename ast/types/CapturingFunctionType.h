// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/TypeLoc.h"

class CapturingFunctionType : public BaseType {
public:

    TypeLoc func_type;

    TypeLoc instance_type;

    /**
     * constructor
     */
    CapturingFunctionType(
        TypeLoc func_type,
        TypeLoc instance_type
    ) : BaseType(BaseTypeKind::CapturingFunction),
        func_type(func_type), instance_type(instance_type)
    {

    }

    FunctionType* get_func_type() {
        return (FunctionType*) func_type.getType();
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<CapturingFunctionType>()) CapturingFunctionType(
            func_type.copy(allocator), instance_type.copy(allocator)
        );
    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::CapturingFunction &&
            func_type->is_same(type->as_capturing_func_type_unsafe()->func_type) &&
            instance_type->is_same(type->as_capturing_func_type_unsafe()->instance_type);
    }

    bool satisfies(BaseType *type) override;

    uint64_t byte_size(bool is64Bit) override {
        return instance_type->byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Type* llvm_param_type(Codegen &gen) override;

#endif


};