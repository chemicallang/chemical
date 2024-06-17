// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
class Codegen;
#endif

class BaseType;

class FunctionParam;

class ExtensionFunction;

class BaseFunctionType {
public:

    std::vector<std::unique_ptr<FunctionParam>> params;
    std::unique_ptr<BaseType> returnType;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;

    /**
     * constructor
     */
    BaseFunctionType(
        std::vector<std::unique_ptr<FunctionParam>> params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic
    );

    virtual ExtensionFunction* as_extension_func() {
        return nullptr;
    }

#ifdef COMPILER_BUILD

    virtual std::vector<llvm::Type *> param_types(Codegen &gen) = 0;

#endif

    /**
     * assigns func_type field of each function parameter to this
     */
    void assign_params();

    /**
     * whether this function requires self parameter
     */
    virtual bool has_self_param();

    /**
     * start index of c or llvm functions for this type
     */
    virtual unsigned c_or_llvm_arg_start_index() const;

    /**
     * check if this function type is equal to other
     */
    bool equal(BaseFunctionType *other) const;

};