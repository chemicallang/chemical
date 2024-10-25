// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public TokenizedBaseType {
public:

    static const VoidType instance;

    using TokenizedBaseType::TokenizedBaseType;

    bool satisfies(ValueType type) final {
        return false;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Void;
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final {
        return false;
    }

    bool satisfies(BaseType *type) final {
        return false;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    virtual VoidType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<VoidType>()) VoidType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};