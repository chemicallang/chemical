// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include <string>

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
class Codegen;
#endif

class BaseType;

class BaseFunctionParam;

class FunctionParam;

class ExtensionFunction;

class FunctionDeclaration;

class Value;

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

    /**
     * check if these args satisfy, this is useful, if calling a constructor
     * user provides the arguments, we check arguments against params, to see if it's compatible
     * if not, another function is selected that is compatible with arguments provided
     */
    virtual bool satisfy_args(std::vector<std::unique_ptr<Value>>& forArgs);

    virtual ExtensionFunction* as_extension_func() {
        return nullptr;
    }

    virtual FunctionDeclaration *as_function() {
        return nullptr;
    }

    /**
     * optional name for the function, used for errors and debugging mostly
     */
    virtual std::string func_opt_name() {
       return "";
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
    virtual BaseFunctionParam* get_self_param();

    /**
     * whether this function requires self parameter
     */
    bool has_self_param() {
        return get_self_param() != nullptr;
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