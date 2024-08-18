// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "ast/base/BaseType.h"

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

class ASTNode;

class FunctionType : public BaseType {
public:

    std::vector<std::unique_ptr<FunctionParam>> params;
    std::unique_ptr<BaseType> returnType = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
    bool isCapturing;

    /**
     * constructor
     */
    FunctionType(
        std::vector<std::unique_ptr<FunctionParam>> params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        bool isCapturing = false
    );

    [[nodiscard]]
    bool is_capturing() const {
        return isCapturing;
    }

    virtual ASTNode* parent() {
        return nullptr;
    }

    /**
     * check if these args satisfy, this is useful, if calling a constructor
     * user provides the arguments, we check arguments against params, to see if it's compatible
     * if not, another function is selected that is compatible with arguments provided
     */
    bool satisfy_args(std::vector<std::unique_ptr<Value>>& forArgs);

    /**
     * suppose this function takes a self argument passed implicitly
     * the index of arguments would start at 1
     */
    unsigned explicit_func_arg_offset();

    /**
     * get function param for argument index
     */
    FunctionParam* func_param_for_arg_at(unsigned index);

    /**
     * do parameter types match with the given function parameter types
     */
    bool do_param_types_match(std::vector<std::unique_ptr<FunctionParam>>& param_types, bool check_self = true);

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Function;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Lambda;
    }

    bool isInVarArgs(unsigned index) const;

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

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

    inline bool equal_type(FunctionType *other) const {
        return equal(other);
    }

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && equal_type(static_cast<FunctionType *>(other));
    }

    bool satisfies(ValueType type) override;

    FunctionType *function_type() override {
        return this;
    }

    [[nodiscard]]
    FunctionType* copy() const override;

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

#ifdef COMPILER_BUILD

    virtual std::vector<llvm::Type *> param_types(Codegen &gen);

    void queue_destruct_params(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

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
    bool equal(FunctionType *other) const;

    /**
     * virtual destructor
     */
    ~FunctionType() override = default;

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