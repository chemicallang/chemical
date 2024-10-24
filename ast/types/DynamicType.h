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

    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }

    bool is_same(BaseType* type) override {
        return type->kind() == BaseTypeKind::Dynamic && ((DynamicType*) type)->referenced->is_same(referenced);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Dynamic;
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Struct;
    }

    bool satisfies(BaseType *type) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

    [[nodiscard]]
    DynamicType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<DynamicType>()) DynamicType(referenced->copy(allocator), token);
    }

    ASTNode* linked_node() override {
        return referenced->linked_node();
    }

    bool link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen& gen) override;

    llvm::Type* llvm_param_type(Codegen& gen) override;

#endif

};