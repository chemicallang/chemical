// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
class Codegen;
#endif

class BaseType;

class BaseFunctionParam;

class FunctionParam;

class ExtensionFunction;

class FunctionDeclaration;

class BaseFunctionType {
public:

    std::vector<std::unique_ptr<FunctionParam>> params;
    std::unique_ptr<BaseType> returnType = nullptr;
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

    virtual FunctionDeclaration* as_func_decl() {
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
     * get the self parameter of the function if it exists
     */
    virtual BaseFunctionParam* get_self_params();

    /**
     * whether this function requires self parameter
     */
    bool has_self_param() {
        return get_self_params() != nullptr;
    }

    /**
     * start index of c or llvm functions for this type
     */
    virtual unsigned c_or_llvm_arg_start_index() const;

    /**
     * check if this function type is equal to other
     */
    bool equal(BaseFunctionType *other) const;

    /**
     * virtual destructor
     */
    virtual ~BaseFunctionType() = default;

};