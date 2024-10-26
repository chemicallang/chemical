// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a negative operator -value
class NegativeValue : public Value {
public:

    Value* value;
    SourceLocation location;

    explicit NegativeValue(Value* value, SourceLocation location) : value(value), location(location) {}

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::NegativeValue;
    }

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    bool primitive() final;

    Value* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NegativeValue>()) NegativeValue(value->copy(allocator), location);
    }

    Value* evaluated_value(InterpretScope &scope) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final {
        return value->llvm_type(gen);
    }

    llvm::Value* llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final;
//    std::unique_ptr<BaseType> create_type() final;

    [[nodiscard]]
    ValueType value_type() const final {
        return value->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return value->type_kind();
    }

};