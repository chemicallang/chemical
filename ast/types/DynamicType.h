// Copyright (c) Qinetik 2024.

#pragma once

#include "LinkedType.h"

class DynamicType : public TokenizedBaseType {
public:

    BaseType* referenced;

    /**
     * constructor
     */
    DynamicType(BaseType* referenced, CSTToken* token);

    void accept(Visitor* visitor) final {
        visitor->visit(this);
    }

    bool is_same(BaseType* type) final {
        return type->kind() == BaseTypeKind::Dynamic && ((DynamicType*) type)->referenced->is_same(referenced);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Dynamic;
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Struct;
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Struct;
    }

    [[nodiscard]]
    DynamicType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<DynamicType>()) DynamicType(referenced->copy(allocator), token);
    }

    ASTNode* linked_node() final {
        return referenced->linked_node();
    }

    bool link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen& gen) final;

    llvm::Type* llvm_param_type(Codegen& gen) final;

#endif

};