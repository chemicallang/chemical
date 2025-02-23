// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public BaseType {
public:

    static const AnyType instance;

    inline AnyType(SourceLocation location) : BaseType(BaseTypeKind::Any, location) {

    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
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
        return new (allocator.allocate<AnyType>()) AnyType(encoded_location());
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};