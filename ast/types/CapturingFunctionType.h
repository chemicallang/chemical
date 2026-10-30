// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/TypeLoc.h"

class CapturingFunctionType : public BaseType {
public:

    TypeLoc func_type;

    TypeLoc instance_type;

    /**
     * The capture environment this callback actually carries, when the type
     * carries a typed witness. A null value means the environment was erased
     * (for example a plain `std::function<F>` declaration without an
     * initializer), which is conservatively neither Send nor Sync.
     *
     * This is a type-level witness only; the runtime layout is still the
     * `instance_type` (the erased function instance). The witness lets auto
     * traits reason about the actual captured fields without changing the
     * ABI.
     */
    TypeLoc capture_env;

    /**
     * constructor
     */
    CapturingFunctionType(
        TypeLoc func_type,
        TypeLoc instance_type,
        TypeLoc capture_env = nullptr
    ) : BaseType(BaseTypeKind::CapturingFunction),
        func_type(func_type), instance_type(instance_type), capture_env(capture_env)
    {

    }

    FunctionType* get_func_type() {
        return (FunctionType*) func_type.getType();
    }

    BaseType* copy(ASTAllocator &allocator) final {
        return new (allocator.allocate<CapturingFunctionType>()) CapturingFunctionType(
            func_type.copy(allocator), instance_type.copy(allocator),
            capture_env ? capture_env.copy(allocator) : TypeLoc(nullptr)
        );
    }

    bool is_same(BaseType *type) override {
        if (type->kind() != BaseTypeKind::CapturingFunction) return false;
        auto* other = type->as_capturing_func_type_unsafe();
        if (!func_type->is_same(other->func_type) ||
            !instance_type->is_same(other->instance_type)) {
            return false;
        }
        if (capture_env == nullptr || other->capture_env == nullptr) {
            return capture_env == nullptr && other->capture_env == nullptr;
        }
        return capture_env->is_same(other->capture_env);
    }

    bool satisfies(BaseType *type) override;

    uint64_t byte_size(const TargetData& target) override {
        return instance_type->byte_size(target);
    }



};