// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// representation is null
class NullValue : public Value {
public:

    BaseType* expected = nullptr;

    /**
     * constructor
     */
    inline explicit constexpr NullValue(SourceLocation location) : Value(ValueKind::NullValue, location) {

    }

    inline NullValue(
        BaseType* expected,
        SourceLocation location
    ) : Value(ValueKind::NullValue, location), expected(expected) {

    }


    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    [[nodiscard]]
    std::string representation() const {
        return "null";
    }

    NullValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NullValue>()) NullValue(expected, encoded_location());
    }

#ifdef COMPILER_BUILD

    static llvm::Value* null_llvm_value(Codegen &gen);

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    bool link(SymbolResolver &linker, BaseType *expected_type = nullptr) final;

    BaseType* create_type(ASTAllocator &allocator) final;

    BaseType* known_type() final;

};