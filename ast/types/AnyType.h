// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public TokenizedBaseType {
public:

    static const AnyType instance;

    using TokenizedBaseType::TokenizedBaseType;

    bool satisfies(ValueType type) final {
        return true;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Any;
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final {
        return true;
    }

    bool satisfies(BaseType *type) final {
        return true;
    }

    bool is_same(BaseType *type) final {
        return true;
    }

    [[nodiscard]]
    AnyType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<AnyType>()) AnyType(token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};