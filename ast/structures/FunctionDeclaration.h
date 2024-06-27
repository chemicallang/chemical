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
#include "ast/base/AccessSpecifier.h"
#include "ast/base/AnnotableNode.h"
#include "BaseFunctionType.h"

class FunctionDeclaration : public AnnotableNode, public BaseFunctionType {
private:
    Value *interpretReturn = nullptr;
    // if the function is variadic, the last type in params is the type given to the variadic parameter

public:

    AccessSpecifier specifier;
    std::string name; ///< The name of the function;
    std::optional<LoopScope> body; ///< The body of the function.
    InterpretScope *declarationScope;

#ifdef COMPILER_BUILD
    llvm::FunctionType *funcType;
    llvm::Value *funcCallee;
#endif

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
            std::vector<std::unique_ptr<FunctionParam>> params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            std::optional<LoopScope> body = std::nullopt
    );

    std::string ns_node_identifier() override {
        return name;
    }

    std::string func_opt_name() override {
        return name;
    }

    void accept(Visitor *visitor) override;

    void ensure_constructor(StructDefinition* def);

    void ensure_destructor(StructDefinition* def);

#ifdef COMPILER_BUILD

    llvm::Function *llvm_func();

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    std::vector<llvm::Type *> param_types(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    /**
     * called by struct when the function is inside a struct
     */
    void code_gen_struct(Codegen &gen, StructDefinition* def);

    /**
     * called by union when the function is inside a union
     */
    void code_gen_union(Codegen &gen, UnionDef* def);

    /**
     * called by interface when the function is inside a interface
     */
    void code_gen_interface(Codegen &gen, InterfaceDefinition* def);

    /**
     * codegen destructor
     */
    void code_gen_destructor(Codegen& gen, StructDefinition* def);

    /**
     * codegen constructor
     */
    void code_gen_constructor(Codegen& gen, StructDefinition* def);

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

    hybrid_ptr<BaseType> get_value_type() override;

    // called by the return statement
    void set_return(Value *value);

    FunctionDeclaration *as_function() override;

};