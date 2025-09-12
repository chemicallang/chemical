// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/types/StringType.h"
#include "std/chem_string_view.h"

/**
 * @brief Class representing a string value.
 */
class StringValue : public Value {
public:

    chem::string_view value;
    unsigned int length = 0;
    bool is_array = false;

    /**
     * constructor
     */
    constexpr StringValue(
        chem::string_view value,
        StringType* strType,
        SourceLocation location
    ) : Value(ValueKind::String, strType, location), length(value.size()), value(value) {

    }

    inline StringType* getType() const noexcept {
        return (StringType*) Value::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *index(InterpretScope &scope, int i) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

#endif

    StringValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<StringValue>()) StringValue(value, getType(), encoded_location());
    }

};