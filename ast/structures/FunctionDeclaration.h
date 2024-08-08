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
#include "GenericTypeParameter.h"

class FunctionDeclaration : public AnnotableNode, public BaseFunctionType {
private:
    Value *interpretReturn = nullptr;
public:

    AccessSpecifier specifier;
    std::string name;
    std::vector<std::unique_ptr<GenericTypeParameter>> generic_params;
    std::optional<LoopScope> body;
    ASTNode* parent_node;
    /**
     * if this is a generic function (it has generic parameters), generic parameters
     * pretend to be different types on different iterations, iterations are number of usages
     * that we determined during symbol resolution, by default zero means no active
     */
    int16_t active_iteration = 0;
    /**
     * when involved in multi function node (due to same name, different parameters)
     */
    uint8_t multi_func_index = 0;
    /**
     * this index corresponds to number of iterations in llvm_data, for which function bodies
     * have been generated, so next time we should start at this index to generate bodies
     */
    int16_t bodies_gen_index = 0;

#ifdef COMPILER_BUILD
    std::vector<std::pair<llvm::Value*, llvm::FunctionType*>> llvm_data;
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
            ASTNode* parent_node,
            std::optional<LoopScope> body = std::nullopt
    );

    /**
     * how many actual functions are generated from this generic function
     * non-generic functions return 1
     */
    int16_t total_generic_iterations();

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    bool is_generic() {
        return !generic_params.empty();
    }

    ASTNode *parent() override {
        return parent_node;
    }

    std::string ns_node_identifier() override {
        return name;
    }

    std::string func_opt_name() override {
        return name;
    }

    // whether the function is exported to other modules (@api)
    bool is_exported();

    void accept(Visitor *visitor) override;

    void ensure_constructor(StructDefinition* def);

    void ensure_destructor(StructDefinition* def);

    using BaseFunctionType::as_extension_func;

    /**
     * set's the active iteration for a generic function
     * this helps generics types pretend to be certain type
     */
    void set_active_iteration(int16_t iteration);

    /**
     * set's the generic active iteration safely
     */
    inline void set_active_iteration_safely(int16_t itr) {
        if(itr < -1) return;
        set_active_iteration(itr);
    }

    /**
     * a call notifies a function, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    int16_t register_call(SymbolResolver& resolver, FunctionCall* call);

#ifdef COMPILER_BUILD

    llvm::Function *llvm_func();

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    std::vector<llvm::Type *> param_types(Codegen &gen) override;

    llvm::FunctionType *create_llvm_func_type(Codegen &gen);

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    /**
     * called by struct to declare functions, so they can be cal
     */
    void code_gen_declare(Codegen &gen, StructDefinition* def);

    /**
     * called by struct definition to generate body for already declared
     * function
     */
    void code_gen_body(Codegen &gen, StructDefinition* def);

    /**
     * called by union when the function is inside a union
     */
    void code_gen_union(Codegen &gen, UnionDef* def);

    /**
     * called by interface when the function is inside a interface
     */
    void code_gen_interface(Codegen &gen, InterfaceDefinition* def);

    /**
     * codegen destructor is called by function declaration itself
     * when a destructor's body is to be generated, mustn't be called
     * by outside functions
     */
    void code_gen_destructor(Codegen& gen, StructDefinition* def);

    /**
     * when normal functions occur in file, this function is called
     */
    void code_gen(Codegen &gen) override;

    /**
     * generic code gen
     */
    void code_gen_generic(Codegen &gen) override;

    /**
     * this function is used to declare the function before generating code for its body
     */
    void code_gen_declare(Codegen &gen) override;

    /**
     * (this) is the function that is overriding
     * the given decl is the function that is being overridden
     * in the struct when a function is overriding another function, this method is
     * called on the overrider with the function that is being overridden
     */
    void code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl);

    /**
     * called when a struct overrides a function declared in interface
     * whereas this function is the function that is overriding the function
     * that is being passed in as a parameter, the function being passed in the parameter
     * is present in an interface (so it can be overridden)
     */
    void code_gen_override(Codegen& gen, FunctionDeclaration* decl);

#endif

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    virtual Value *call(
        InterpretScope *call_scope,
        FunctionCall* call,
        Value* parent_val
    );

    virtual Value *call(
        InterpretScope *call_scope,
        std::vector<std::unique_ptr<Value>> &call_args,
        Value* parent_val,
        InterpretScope *fn_scope
    );

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    // called by the return statement
    void set_return(Value *value);

    FunctionDeclaration *as_function() override;

};