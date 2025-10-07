// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

/**
 * a runtime type is a value type that comes from non comptime context
 * for example func sum(a : %runtime<*int>)
 * here when you call sum, the pointer is not present in a comptime context
 */
class RuntimeType : public BaseType {
public:

    BaseType* underlying;

    /**
     * constructor
     */
    constexpr RuntimeType(BaseType* underlying) : underlying(underlying), BaseType(BaseTypeKind::Runtime) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying->byte_size(is64Bit);
    }

    bool satisfies(BaseType *type) override {
        if(type->kind() == BaseTypeKind::Runtime) {
            return underlying->satisfies(type->as_runtime_type_unsafe()->underlying);
        } else {
            return underlying->satisfies(type);
        }
    }

    bool satisfies(Value* value, bool assignment) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Runtime && ((RuntimeType*) type)->underlying->is_same(underlying);
    }

    [[nodiscard]]
    RuntimeType* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<RuntimeType>()) RuntimeType(underlying->copy(allocator));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final {
        return underlying->llvm_type(gen);
    }
#endif

};