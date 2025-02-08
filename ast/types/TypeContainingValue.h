// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

/**
 * TypeContainingValue is a type that contains a value (which actually will return a type)
 * HOW ? this type cannot be directly used everywhere
 * It is used with typealias statement, typealias will only replace it in comptime mode
 * when it's interpret method is called, if code is being generated, typealias will create
 * an error instead, if it contains this type
 */
class TypeContainingValue : public BaseType {
public:

    Value* provider;

    explicit inline TypeContainingValue(Value* provider) : provider(provider) {

    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::TypeContainingValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(BaseType *type) override {
        return false;
    }

    bool is_same(BaseType *type) override {
        return false;
    }

    SourceLocation encoded_location() override {
        return provider->encoded_location();
    }

    BaseType * copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<TypeContainingValue>()) TypeContainingValue(provider);
    }

    bool link(SymbolResolver &linker) override {
        return provider->link(linker, provider, nullptr);
    }

};