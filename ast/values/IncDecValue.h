// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"

class IncDecValue : public Value {
public:

    Value* value;
    bool increment;
    bool post;

    /**
     * constructor
     */
    constexpr IncDecValue(
        Value* value,
        bool increment,
        bool post,
        SourceLocation location
    ) : Value(ValueKind::IncDecValue, location), value(value), increment(increment), post(post) {

    }

    BaseType* create_type(ASTAllocator &allocator) override;

    bool link(SymbolResolver &linker, BaseType *expected_type) override {
        return value->link(linker, expected_type);
    }

    Value* evaluated_value(InterpretScope &scope) override;

    IncDecValue* copy(ASTAllocator &allocator) final {
        return new (allocator.allocate<IncDecValue>()) IncDecValue(
            value->copy(allocator),
            increment,
            post,
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override {
        return value->llvm_type(gen);
    }

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif


};