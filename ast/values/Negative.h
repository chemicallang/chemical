// Copyright (c) Chemical Language Foundation 2025.

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

    /**
     * constructor
     */
    constexpr NegativeValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NegativeValue, location), value(value) {

    }

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final;

    bool link(SymbolResolver &linker, BaseType *expected_type = nullptr) final;

    bool primitive() final;

    Value* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NegativeValue>()) NegativeValue(value->copy(allocator), encoded_location());
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

};