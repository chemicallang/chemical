// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

/**
 * a parameter is added to the function implicitly when it returns a struct
 * when user returns a struct the returning struct is copied into this implicitly
 * passed struct, with this value user can gain access to implicitly passed struct
 * and then modify that
 */
class RetStructParamValue : public Value {
public:

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    hybrid_ptr<BaseType> get_base_type() override;

    std::unique_ptr<BaseType> create_type() override;

    Value *copy() override {
        return new RetStructParamValue();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Pointer;
    }

};