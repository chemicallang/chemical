// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"

class IncDecValue : public Value {
public:

    Value* value;
    bool increment;
    bool post;
    SourceLocation location;

    IncDecValue(
        Value* value,
        bool increment,
        bool post,
        SourceLocation location
    ) : value(value), increment(increment), post(post), location(location) {

    }

    ValueKind val_kind() override {
        return ValueKind::IncDecValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    SourceLocation encoded_location() override {
        return location;
    }

    BaseType* create_type(ASTAllocator &allocator) override {
        return value->create_type(allocator);
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) override {
        return value->link(linker, value_ptr, expected_type);
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override {
        return value->llvm_type(gen);
    }

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif


};