// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// representation is null
class NullValue : public Value {
public:

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    std::string representation() const {
        return "null";
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

};