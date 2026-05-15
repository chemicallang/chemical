// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "ast/base/TypeLoc.h"
#include "ast/base/ValueKind.h"
#include "ast/structures/FunctionParam.h"

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
#include "ast/base/AnnotationKind.h"

class Codegen;
#endif

class BaseType;

class FunctionParam;

class ExtensionFunction;

class FunctionDeclaration;

class Value;

class ASTNode;

class ASTDiagnoser;

struct FunctionTypeData {
    /**
     * extension functions are same as functions, however are children of containers
     */
    bool isExtension = false;
    /**
     * is the function variadic
     */
    bool isVariadic = false;
    /**
     * this is marked true, when symbol resolution of the signature completes successfully
     */
    bool signature_resolved = false;
    /**
     * is the function capturing
     */
    bool isCapturing = false;
};

static_assert(sizeof(FunctionTypeData) <= 8);

class FunctionDeclaration;

class FunctionType : public BaseType {
public:

    std::vector<FunctionParam*> params;
    TypeLoc returnType = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    /**
     * function type data
     */
    FunctionTypeData data;

    /**
     * constructor
     */
    constexpr FunctionType(
            TypeLoc returnType,
            bool isVariadic,
            bool isCapturing,
            bool signature_resolved
    ) : data(false, isVariadic, signature_resolved, isCapturing), returnType(returnType),
        BaseType(BaseTypeKind::Function) {

    }

    /**
     * constructor
     */
    constexpr FunctionType(
            TypeLoc returnType,
            bool isExtension,
            bool isVariadic,
            bool isCapturing,
            bool signature_resolved
    ) : data(isExtension, isVariadic, signature_resolved, isCapturing), returnType(returnType),
        BaseType(BaseTypeKind::Function) {

    }


    [[nodiscard]]
    inline bool isExtensionFn() const {
        return data.isExtension;
    }

    inline void setIsExtension(bool value) {
        data.isExtension = value;
    }

    [[nodiscard]]
    inline bool isCapturing() const {
        return data.isCapturing;
    }

    [[nodiscard]]
    inline bool isVariadic() const {
        return data.isVariadic;
    }

    inline void setIsCapturing(bool capturing) {
        data.isCapturing = capturing;
    }

    inline void setIsVariadic(bool variadic) {
        data.isVariadic = variadic;
    }

    unsigned int getStructReturnArgIndex() {
        return isCapturing() ? 1 : 0;
    }

    /**
     * copy this function type into another
     */
    void shallow_copy_into(FunctionType& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.params = params;
        other.returnType = returnType;
        other.data = data;
    }

    /**
     * copy this function type into another
     */
    void copy_into(FunctionType& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.params.reserve(params.size());
        for(auto& param : params) {
            const auto new_param = param->copy(allocator);
            new_param->set_parent(new_parent);
            other.params.emplace_back(new_param);
        }
        other.returnType = returnType.copy(allocator);
        other.data = data;
    }

    /**
     * check if these args satisfy, this is useful, if calling a constructor
     * user provides the arguments, we check arguments against params, to see if it's compatible
     * if not, another function is selected that is compatible with arguments provided
     */
    bool satisfy_args(std::vector<Value*>& forArgs);

    /**
     * get the total implicit parameters
     */
    unsigned total_implicit_params();

    /**
     * suppose this function takes a self argument passed implicitly
     * the index of arguments would start at 1
     */
    inline unsigned explicit_func_arg_offset() {
        const auto ext_offset = isExtensionFn() ? 1 : 0;
        return total_implicit_params() + ext_offset;
    }

    /**
     * get function param for argument index
     */
    FunctionParam* func_param_for_arg_at(unsigned index);

    /**
     * get implicit parameter for given name
     */
    FunctionParam* implicit_param_for(const chem::string_view& name);

    /**
     * do parameter types match with the given function parameter types
     */
    bool do_param_types_match(std::vector<FunctionParam*>& param_types, bool check_self = true);

    bool isInVarArgs(unsigned index) const;

    unsigned int expectedArgsSize();

    uint64_t byte_size(TargetData& target) final;

    ASTNode* linked_node() override {
        return returnType->linked_node();
    }

    virtual ExtensionFunction* as_extension_func() {
        return nullptr;
    }

    virtual FunctionDeclaration *as_function() {
        return nullptr;
    }

    virtual LambdaFunction* as_lambda() {
        return nullptr;
    }

    inline bool equal_type(FunctionType *other) const {
        return equal(other);
    }

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && equal_type(static_cast<FunctionType *>(other));
    }

    bool satisfies(BaseType *type) override;

    [[nodiscard]]
    FunctionType* copy(ASTAllocator& allocator) override;

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> param_types(Codegen &gen, bool capturing);

    inline std::vector<llvm::Type *> param_types(Codegen &gen) {
        return param_types(gen, isCapturing());
    }

    void queue_destruct_params(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen);

private:

    llvm::FunctionType *llvm_func_type(Codegen &gen, bool capturing);

public:

    inline llvm::FunctionType *llvm_func_type(Codegen &gen) {
        return llvm_func_type(gen, isCapturing());
    }

    inline llvm::FunctionType* llvm_capturing_func_type(Codegen& gen) {
        return llvm_func_type(gen, true);
    }

#endif

    /**
     * get the self parameter of the function if it exists
     */
    FunctionParam* get_self_param();

    /**
     * whether this function requires self parameter
     */
    bool has_self_param() {
        return get_self_param() != nullptr;
    }

    /**
     * does function has explicit parameters
     */
    bool has_explicit_params();

    /**
     * start index of c or llvm functions for this type
     */
    unsigned c_or_llvm_arg_start_index();

    /**
     * check if this function type is equal to other
     */
    bool equal(FunctionType *other) const;

    /**
     * virtual destructor
     */
    ~FunctionType() override = default;

};

/**
 * this should only be inherited by functions or lambdas
 * those that have bodies
 */
class FunctionTypeBody : public FunctionType {
public:

    using FunctionType::FunctionType;

    /**
     * get the parent node
     */
    virtual ASTNode* get_parent() = 0;

    /**
     * get the encoded location
     */
    virtual SourceLocation get_location() = 0;

    /**
       * copy function type body into the other
       */
    inline void shallow_copy_into(FunctionTypeBody& other, ASTAllocator& allocator, ASTNode* new_parent) {
        FunctionType::shallow_copy_into(other, allocator, new_parent);
    }

    /**
     * copy function type body into the other
     */
    inline void copy_into(FunctionTypeBody& other, ASTAllocator& allocator, ASTNode* new_parent) {
        FunctionType::copy_into(other, allocator, new_parent);
    }

    /**
     * called by return statement to set the return value and stop
     * interpretation of function body
     */
    virtual void set_return(InterpretScope& scope, Value* value) = 0;

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
        std::vector<FunctionParam*>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic,
        FunctionDeclaration* decl
);

/**
 * creates llvm function parameter types, based on our parameter types of the function
 * a helper to call function defined above it
 */
inline std::vector<llvm::Type*> llvm_func_param_types(
        Codegen &gen,
        std::vector<FunctionParam*>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic,
        FunctionDeclaration* decl
) {
    std::vector<llvm::Type*> paramTypes;
    llvm_func_param_types_into(gen, paramTypes, params, returnType, isCapturing, isVariadic, decl);
    return paramTypes;
}

#endif