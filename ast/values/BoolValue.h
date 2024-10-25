// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class BoolValue : public Value {
public:

    bool value;
    SourceLocation location;

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    explicit BoolValue(bool value, SourceLocation location) : value(value), location(location) {

    }

    ValueKind val_kind() final {
        return ValueKind::Bool;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    BoolValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, location);
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &BoolType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<BoolType>()) BoolType(location);
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Bool;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Bool;
    }

};