// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"

class ReferencedType : public BaseType {
public:

    std::string type;

    ASTNode *linked;

    ReferencedType(std::string type) : type(std::move(type)) {}

    ReferencedType(std::string type, ASTNode* linked) : type(std::move(type)), linked(linked) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ValueType value_type() const override;

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    bool satisfies(ValueType value_type) const override;

    BaseTypeKind kind() const override {
        return BaseTypeKind::Referenced;
    }

    bool is_same(BaseType *other) const override {
        return other->kind() == kind() && static_cast<ReferencedType *>(other)->type == type;
    }

    std::string representation() const override {
        return type;
    }

    virtual BaseType *copy() const {
        auto t = new ReferencedType(type);
        t->linked = linked;
        return t;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};