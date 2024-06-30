// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/BaseFunctionType.h"

using func_params = std::vector<std::unique_ptr<FunctionParam>>;

class FunctionType : public BaseFunctionType, public BaseType {
public:

    bool isCapturing;

    FunctionType(
            std::vector<std::unique_ptr<FunctionParam>> params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            bool isCapturing
    );

    ASTNode *parent() override {
        return nullptr;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    bool isInVarArgs(unsigned index);

    BaseTypeKind kind() const override {
        return BaseTypeKind::Function;
    }

    ValueType value_type() const override {
        return ValueType::Lambda;
    }

    inline bool equal_type(FunctionType *other) const {
        return equal(other);
    }

    bool is_same(BaseType *other) const override {
        return other->kind() == kind() && equal_type(static_cast<FunctionType *>(other));
    }

    bool satisfies(ValueType type) const override;

    FunctionType *function_type() override {
        return this;
    }

    virtual BaseType *copy() const;

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

#endif

};

#ifdef COMPILER_BUILD

/**
 * creates llvm function return type, based on our return type of the function
 */
llvm::Type* llvm_func_return(Codegen &gen, BaseType* type);

/**
 * add a single param type to param types
 */
void llvm_func_param_type(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        BaseType* type
);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 */
void llvm_func_param_types_into(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        std::vector<std::unique_ptr<FunctionParam>>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic
);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 * a helper to call function defined above it
 */
inline std::vector<llvm::Type*> llvm_func_param_types(
        Codegen &gen,
        std::vector<std::unique_ptr<FunctionParam>>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic
) {
    std::vector<llvm::Type*> paramTypes;
    llvm_func_param_types_into(gen, paramTypes, params, returnType, isCapturing, isVariadic);
    return paramTypes;
}

#endif