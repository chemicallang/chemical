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
#include "ast/types/FunctionType.h"
#include "GenericTypeParameter.h"
#include "ast/base/LocatedIdentifier.h"

class ExtendableMembersContainerNode;

/**
 * Function declaration's extra data
 * this is stored here, to avoid making declarations memory size insane
 */
struct FuncDeclExtData {
    /**
     * the access specifier for the function declaration
     */
    AccessSpecifier specifier = AccessSpecifier::Internal;
    /**
     * set to true after declare_top_level, if signature resolved successfully
     */
    bool resolved_signature_successfully = false;
    /**
     * when involved in multi function node (due to same name, different parameters)
     */
    uint8_t multi_func_index = 0;
};

static_assert(sizeof(FuncDeclExtData) <= 8);

class FunctionDeclaration : public AnnotableNode, public FunctionType {
private:
    /**
     * TODO avoid storing the interpret return here
     */
    Value *interpretReturn = nullptr;
    /**
     * TODO call scope shouldn't be stored here
     */
    InterpretScope* callScope = nullptr;
public:

    /**
     * the name of the function is stored inside
     */
    LocatedIdentifier identifier;
    /**
     * the generic parameters
     */
    std::vector<GenericTypeParameter*> generic_params;
    /**
     * subscribers are notified of generic usages of this function
     */
    std::vector<GenericType*> subscribers;
    /**
     * optional body
     */
    std::optional<LoopScope> body;
    /**
     * if this is a generic function (it has generic parameters), generic parameters
     * pretend to be different types on different iterations, iterations are number of usages
     * that we determined during symbol resolution
     */
    int16_t active_iteration = 0;
    /**
     * this index corresponds to number of iterations in llvm_data, for which function bodies
     * have been generated, so next time we should start at this index to generate bodies
     */
    int16_t bodies_gen_index = 0;
    /**
     * the location
     */
    SourceLocation location;

    /**
     * the llvm data
     */
#ifdef COMPILER_BUILD
    std::vector<std::pair<llvm::Value*, llvm::FunctionType*>> llvm_data;
#endif

    /**
     * external data is stored in
     */
    FuncDeclExtData data;

    /**
     * constructor
     */
    FunctionDeclaration(
            LocatedIdentifier identifier,
            std::vector<FunctionParam*> params,
            BaseType* returnType,
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location,
            std::optional<LoopScope> body = std::nullopt,
            AccessSpecifier = AccessSpecifier::Internal
    );

    inline AccessSpecifier specifier() {
        return data.specifier;
    }

    inline void set_specifier_fast(AccessSpecifier specifier) {
        data.specifier = specifier;
    }

    inline const std::string& name() const {
        return identifier.identifier;
    }

    inline uint8_t multi_func_index() {
        return data.multi_func_index;
    }

    inline void set_multi_func_index(uint8_t i) {
        data.multi_func_index = i;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNodeKind kind() {
        return ASTNodeKind::FunctionDecl;
    }

    /**
     * how many actual functions are generated from this generic function
     * non-generic functions return 1
     */
    int16_t total_generic_iterations();

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    bool is_generic() {
        return !generic_params.empty();
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    std::string runtime_name_fast() {
        return parent_node ? runtime_name_str() : runtime_name_no_parent_fast_str();
    }

    void runtime_name_no_parent_fast(std::ostream &stream);

    std::string runtime_name_no_parent_fast_str();

    void runtime_name_no_parent(std::ostream &stream) final {
        return runtime_name_no_parent_fast(stream);
    }

    void runtime_name(std::ostream &stream) final;

    ASTNode *parent() final {
        return parent_node;
    }

    const std::string& ns_node_identifier() final {
        return name();
    }

    const std::string& func_opt_name() final {
        return name();
    }

    void accept(Visitor *visitor);

    void make_destructor(ASTAllocator&, ExtendableMembersContainerNode* def);

    void ensure_constructor(SymbolResolver& resolver, StructDefinition* def);

    void ensure_destructor(SymbolResolver& resolver, ExtendableMembersContainerNode* def);

    void ensure_clear_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def);

    void ensure_copy_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def);

    void ensure_move_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def);

    using FunctionType::as_extension_func;

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
    int16_t register_call(SymbolResolver& resolver, FunctionCall* call, BaseType* expected_type);

    /**
     * called by generic types to subscribe to generic usages of this function
     */
    void subscribe(GenericType *subscriber) final {
        subscribers.emplace_back(subscriber);
    }

    /**
     * get known function type, which is this
     */
    BaseType* known_type() final {
        return this;
    }

#ifdef COMPILER_BUILD

    std::pair<llvm::Value*, llvm::FunctionType*>& get_llvm_data();

    llvm::Function *llvm_func();

    llvm::Value* llvm_callee();

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_load(Codegen &gen) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    std::vector<llvm::Type *> param_types(Codegen &gen);

    llvm::FunctionType *create_llvm_func_type(Codegen &gen);

    /**
     * this function will take into account @cpp annotation
     */
    std::string runtime_name_fast(Codegen& gen);

    /**
     * this function returns known llvm function type, this means
     * that function has been created before, so func type is known for current generic
     * iteration
     */
    llvm::FunctionType *known_func_type();

    /**
     * this will get the func type, no matter what, if it doesn't exist
     * it will create the function type and store it
     */
    llvm::FunctionType *llvm_func_type(Codegen &gen) final;

    /**
     * given llvm data will be set for active iteration
     */
    void set_llvm_data(llvm::Value* func_callee, llvm::FunctionType* func_type);

    /**
     * declare a function that is present inside struct definition
     */
    void code_gen_declare(Codegen &gen, StructDefinition* def);

    /**
     * declare a function that is present inside variant definition
     */
    void code_gen_declare(Codegen &gen, VariantDefinition* def);

    /**
     * declare a function that is present inside a interface definition
     */
    void code_gen_declare(Codegen &gen, InterfaceDefinition* def);

    /**
     * called by union to declare this function
     */
    void code_gen_declare(Codegen &gen, UnionDef* def);

    /**
     * called by struct definition to generate body for already declared
     * function
     */
    void code_gen_body(Codegen &gen, StructDefinition* def);

    /**
     * generate body for the function present inside a variant definition
     */
    void code_gen_body(Codegen &gen, VariantDefinition* def);

    /**
     * called by union when the function is inside a union
     */
    void code_gen_body(Codegen &gen, UnionDef* def);

    /**
     * called by interface when the function is inside a interface
     */
    void code_gen_body(Codegen &gen, InterfaceDefinition* def);

    /**
     * setup cleanup block for the destructor, it'll set it as insert point
     */
    void setup_cleanup_block(Codegen& gen, llvm::Function* func);

    /**
     * codegen destructor is called by function declaration itself
     * when a destructor's body is to be generated, mustn't be called
     * by outside functions
     */
    void code_gen_destructor(Codegen& gen, StructDefinition* def);

    /**
     * body for the copy function is generated using this function
     * the copy function is a special function that will call copy functions
     * of the struct members that require it
     */
    void code_gen_copy_fn(Codegen& gen, StructDefinition* def);

    /**
     * body for the move function is generated using this function
     * the copy function is a special function that will call pre move functions
     * of the struct members that require it
     */
    void code_gen_move_fn(Codegen& gen, StructDefinition* def);

    /**
     * body for the move function is generated using this function
     * the move function is a special function that will call move functions
     * of the struct members that require it
     */
    void code_gen_clear_fn(Codegen& gen, StructDefinition* def);

    /**
     * body for the copy function is generated using this function
     * the copy function is a special function that will call copy functions
     * of the struct members that require it
     */
    void code_gen_copy_fn(Codegen& gen, VariantDefinition* def);

    /**
     * body for the move function is generated using this function
     * the move function is a special function that will call pre move functions
     * of the struct members that require it
     */
    void code_gen_move_fn(Codegen& gen, VariantDefinition* def);

    /**
     * generates clear_fn body for the variant definition
     */
    void code_gen_clear_fn(Codegen& gen, VariantDefinition* def);

    /**
     * generates destructor body for the variant definition
     */
    void code_gen_destructor(Codegen& gen, VariantDefinition* def);

    /**
     * generate body of the function for normal functions
     */
    void code_gen_body(Codegen &gen);

    /**
     * this function declares normal functions
     */
    void code_gen_declare_normal(Codegen& gen);

    /**
     * this function is used to declare the function before generating code for its body
     */
    void code_gen_declare(Codegen &gen) final;

    /**
     * this function generates the body of the function
     * for any function (present inside struct / variant, or a normal function)
     */
    void code_gen(Codegen &gen) final;

    /**
     * generic code gen
     */
    void code_gen_generic(Codegen &gen) final;

    /**
     * (this) is the function that is overriding
     * the given decl is the function that is being overridden
     * in the struct when a function is overriding another function, this method is
     * called on the overrider with the function that is being overridden
     */
    void code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl);

    /**
     * use this function's body to override a llvm function
     * this function will get the entry block from given llvm function
     * and print it's body onto it, the function signatures should match
     * otherwise it won't work
     */
    void code_gen_override(Codegen& gen, llvm::Function* llvm_func);

    /**
     * called to externally declare the function, taking into account it's parent node
     */
    void code_gen_external_declare(Codegen &gen) final;

#endif

    void declare_top_level(SymbolResolver &linker);

    void redeclare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker);

    /**
     * ensure that function body has an init block (required in constructors)
     * @return true if has init block, otherwise false
     */
    bool ensure_has_init_block();

    /**
     * a helper function that would put an error
     */
    void ensure_has_init_block(ASTDiagnoser& diagnoser);

    virtual Value *call(
        InterpretScope *call_scope,
        ASTAllocator& allocator,
        FunctionCall* call,
        Value* parent_val,
        bool evaluate_refs = true
    );

    Value *call(
        InterpretScope *call_scope,
        std::vector<Value*> &call_args,
        Value* parent_val,
        InterpretScope *fn_scope,
        bool evaluate_refs = true,
        ASTAny* debug_value = nullptr
    );

    BaseType* create_value_type(ASTAllocator& allocator);

    // called by the return statement
    void set_return(InterpretScope& func_scope, Value *value);

    FunctionDeclaration *as_function() final;

};