// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

/**
 * a maybe runtime type allows compile time and runtime
 * arguments in a comptime function, for example:
 * comptime func callit(arg : %maybe_runtime<int>)
 * now we can call 'callit' using a compile time argument like '5' or a runtime argument like this
 * func delegate(my_arg : int) { callit(my_arg) } <-- not a comptime function, my_arg could be anything
 */
class MaybeRuntimeType : public BaseType {
public:

    BaseType* underlying;

    /**
     * constructor
     */
    constexpr MaybeRuntimeType(BaseType* underlying) : underlying(underlying), BaseType(BaseTypeKind::MaybeRuntime) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(BaseType *type) override {
        if(type->kind() == BaseTypeKind::MaybeRuntime) {
            return underlying->satisfies(type->as_maybe_runtime_type_unsafe()->underlying);
        } else {
            return underlying->satisfies(type);
        }
    }

    bool satisfies(Value* value, bool assignment) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::MaybeRuntime && ((MaybeRuntimeType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    MaybeRuntimeType* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<MaybeRuntimeType>()) MaybeRuntimeType(underlying->copy(allocator));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final {
        return underlying->llvm_type(gen);
    }
#endif

};