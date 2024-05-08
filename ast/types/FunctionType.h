// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/structures/FunctionParam.h"

class FunctionType : public BaseType {
public:

    func_params params;
    std::unique_ptr<BaseType> returnType;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
    bool isCapturing;

    FunctionType(
            func_params params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            bool isCapturing
    );

    BaseTypeKind kind() const override {
        return BaseTypeKind::Function;
    }

    ValueType value_type() const override {
        return ValueType::Lambda;
    }

    bool equal(FunctionType *other) const {
        if (isVariadic != other->isVariadic) {
            return false;
        }
        if (!returnType->is_same(other->returnType.get())) {
            return false;
        }
        unsigned i = 0;
        while (i < params.size()) {
            if (!params[i]->type->is_same(other->params[i]->type.get())) {
                return false;
            }
            i++;
        }
        return true;
    }

    bool is_same(BaseType *other) const override {
        return other->kind() == kind() && equal(static_cast<FunctionType *>(other));
    }

    bool satisfies(ValueType type) const override;

    FunctionType *function_type() override {
        return this;
    }

    std::string representation() const override;

    virtual BaseType *copy() const;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

#endif

};