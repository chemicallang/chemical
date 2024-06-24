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

    /**
     * @brief Construct a new StringValue object.
     *
     * @param value The string value.
     */
    StringValue(std::string value) : value(std::move(value)) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) const override {
        return is64Bit ? 8 : 4;
    }

    Value *index(InterpretScope &scope, int i) override {
#ifdef DEBUG
        if (i < 0 || i >= value.size()) {
            std::cerr << "[InterpretError] access index " + std::to_string(i) + " out of bounds for string " + value +
                         " of length " + std::to_string(value.size());
        }
#endif
        return new CharValue(value[i]);
    }

    std::string as_string() override {
        return value;
    }

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::GlobalVariable * llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) override;

#endif

    Value *copy() override {
        return new StringValue(value);
    }

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<StringType>();
    }

    ValueType value_type() const override {
        return ValueType::String;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::String;
    }

    std::string value; ///< The string value.
};