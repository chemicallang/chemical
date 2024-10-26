// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/values/CharValue.h"
#include "ast/types/StringType.h"

/**
 * @brief Class representing a string value.
 */
class StringValue : public Value {
public:

    std::string value;
    unsigned int length = 0;
    bool is_array = false;
    SourceLocation location;

    /**
     * @brief Construct a new StringValue object.
     *
     * @param value The string value.
     */
    explicit StringValue(std::string value, SourceLocation location) : length(value.size()), value(std::move(value)), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::String;
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &StringType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &StringType::instance;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *index(InterpretScope &scope, int i) final {
#ifdef DEBUG
        if (i < 0 || i >= value.size()) {
            std::cerr << "[InterpretError] access index " + std::to_string(i) + " out of bounds for string " + value +
                         " of length " + std::to_string(value.size());
        }
#endif
        return new CharValue(value[i], location);
    }

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

#endif

    StringValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<StringValue>()) StringValue(value, location);
    }

    BaseType* create_type(ASTAllocator& allocator) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::String;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::String;
    }

};