// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "LoopScope.h"
#include <optional>

class FunctionParam : public ASTNode {
public:

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            bool isVariadic,
            std::optional<std::unique_ptr<Value>> defValue
    );

    std::unique_ptr<BaseType> create_value_type() override {
        return std::unique_ptr<BaseType>(type->copy());
    }

    void accept(Visitor &visitor) override;

    FunctionParam *as_parameter() override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

#endif

    FunctionParam *copy() const;

    std::string representation() const override;

    unsigned int index;
    std::string name;
    std::unique_ptr<BaseType> type;
    bool isVariadic;
    std::optional<std::unique_ptr<Value>> defValue;
};

class FunctionDeclaration : public ASTNode {
public:

    /**
     * @brief Construct a new FunctionDeclaration object.
     *
     * @param name The name of the function.
     * @param returnType The return type of the function.
     * @param parameters The parameters of the function.
     * @param body The body of the function.
     */
    FunctionDeclaration(
            std::string name,
            func_params params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            std::optional<LoopScope> body = std::nullopt
    );

    void accept(Visitor &visitor) override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    std::vector<llvm::Type *> param_types(Codegen &gen);

    llvm::FunctionType *function_type(Codegen &gen);

    /**
     * called by struct when the function is inside a struct
     */
    void code_gen_struct(Codegen &gen);

    void code_gen(Codegen &gen) override;

#endif

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope) override;

    virtual Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params);

    virtual Value *call(
            InterpretScope *call_scope,
            std::vector<std::unique_ptr<Value>> &call_params,
            InterpretScope *fn_scope
    );

    std::unique_ptr<BaseType> create_value_type() override;

    // called by the return statement
    void set_return(Value *value);

    FunctionDeclaration *as_function() override;

    std::string representation() const override;

    AccessSpecifier specifier;
    std::string name; ///< The name of the function.
    func_params params;
    std::optional<LoopScope> body; ///< The body of the function.
    InterpretScope *declarationScope;
    std::unique_ptr<BaseType> returnType;

#ifdef COMPILER_BUILD
    llvm::FunctionType* funcType;
    llvm::Value* funcCallee;
#endif

private:
    Value *interpretReturn = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
};