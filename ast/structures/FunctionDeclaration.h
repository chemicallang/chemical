// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/structures/FunctionParam.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "LoopScope.h"
#include <optional>

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

    llvm::Function *llvm_func();

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    std::vector<llvm::Type *> param_types(Codegen &gen);

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    /**
     * called by struct when the function is inside a struct
     */
    void code_gen_struct(Codegen &gen);

    /**
     * called by interface when the function is inside a interface
     */
    void code_gen_interface(Codegen &gen);

    /**
     * when normal functions occur in file, this function is called
     */
    void code_gen(Codegen &gen) override;

    /**
     * this function is used to declare the function before generating code for its body
     */
    void code_gen_declare(Codegen &gen) override;

    /**
     * called when a struct overrides a function declared in interface
     * whereas this function is the function in interface, and passed is the struct one
     */
    void code_gen_override(Codegen& gen, FunctionDeclaration* decl);

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
    llvm::FunctionType *funcType;
    llvm::Value *funcCallee;
#endif

private:
    Value *interpretReturn = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
};