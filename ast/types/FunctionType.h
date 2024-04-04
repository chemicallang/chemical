// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/structures/FunctionDeclaration.h"

class FunctionType : public BaseType {
public:

    func_params params;
    std::unique_ptr<BaseType> returnType;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;

    FunctionType(
            func_params params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic
    );

    bool satisfies(ValueType type) const override;

    std::string representation() const override;

    virtual BaseType* copy() const;

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};