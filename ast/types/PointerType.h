// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public BaseType {
public:

    std::unique_ptr<BaseType> type;

    PointerType(std::unique_ptr<BaseType> type) : type(std::move(type)) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType value_type) const override {
        return type->satisfies(value_type);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Pointer;
    }

    bool is_same(BaseType *other) const override {
        return other->kind() == kind() && static_cast<PointerType *>(other)->type->is_same(type.get());
    }

    PointerType *pointer_type() override {
        return this;
    }

    std::string representation() const override {
        return type->representation() + "*";
    }

    virtual BaseType *copy() const {
        return new PointerType(std::unique_ptr<BaseType>(type->copy()));
    }

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};