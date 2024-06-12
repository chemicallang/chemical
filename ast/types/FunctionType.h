// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/structures/FunctionParam.h"

using func_params = std::vector<std::unique_ptr<FunctionParam>>;

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

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker) override;

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

    llvm::Value *llvm_return_intercept(Codegen &gen, llvm::Value *value, ASTNode *node) override;

#endif

};

#ifdef COMPILER_BUILD

/**
 * creates llvm function return type, based on our return type of the function
 */
llvm::Type* llvm_func_return(Codegen &gen, BaseType* type);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 */
std::vector<llvm::Type*> llvm_func_param_types(
        Codegen &gen,
        std::vector<std::unique_ptr<FunctionParam>>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic
);

#endif