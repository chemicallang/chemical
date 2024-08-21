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
    unsigned int length;
    bool is_array = false;

    /**
     * @brief Construct a new StringValue object.
     *
     * @param value The string value.
     */
    explicit StringValue(std::string value) : length(value.size()), value(std::move(value)) {}

    void link(SymbolResolver &linker, VarInitStatement *stmnt) override;

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &StringType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &StringType::instance;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) override {
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

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) override;

    llvm::GlobalVariable * llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) override;

#endif

    StringValue *copy() override {
        return new StringValue(value);
    }

    std::unique_ptr<BaseType> create_type() override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::String;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::String;
    }

};