// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/structures/FunctionParam.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "Scope.h"
#include <optional>
#include "ast/base/AccessSpecifier.h"
#include "ast/base/AnnotableNode.h"
#include "ast/types/FunctionType.h"
#include "GenericTypeParameter.h"
#include "ast/base/LocatedIdentifier.h"
#include <unordered_map>

class ExtendableMembersContainerNode;

/**
 * Function declaration's extra data
 * this is stored here, to avoid making declarations memory size insane
 */
struct FuncDeclAttributes {
    /**
     * the access specifier for the function declaration
     */
    AccessSpecifier specifier = AccessSpecifier::Internal;
    /**
     * is this function comptime
     */
    bool is_comptime;
    /**
     * compiler functions are present inside the compiler
     * like compiler::println
     */
    bool is_compiler_decl;
    /**
     * when involved in multi function node (due to same name, different parameters)
     */
    uint8_t multi_func_index = 0;
    /**
     * is the function marked inline
     */
    bool is_inline;
    /**
     * is the function marked always_inline
     */
    bool always_inline;
    /**
     * is the function marked no_inline
     */
    bool no_inline;
    /**
     * is the function marked inline_hint
     */
    bool inline_hint;
    /**
     * is the function marked opt_size
     */
    bool opt_size;
    /**
     * is the function marked min_size
     */
    bool min_size;
    /**
     * the strongest inline, compiler inline is done by our compiler
     * upon failure an error is generated
     */
    bool compiler_inline;
    /**
     * is function external (in another module), user marks a function
     * external and we basically declare it like that
     */
    bool is_extern;

    /**
     * should mangle as a C++ function
     */
    bool is_cpp_mangle;

    /**
     * is this function deprecated
     */
    bool deprecated;

    /**
     * implicit constructor annotation, allows for automatic type conversion
     */
    bool is_implicit;

    /**
     * is function no return (doesn't return, abort and other functions)
     */
    bool is_noReturn;
    /**
     * is constructor function
     */
    bool is_constructor_fn;
    /**
     * a move function is triggered on the object that has been moved (it's not like C++ move constructor which is called on the newly object being constructed)
     * it means to say, function that defines what happens when the object is moved and NOT how to construct an object from another object without copying everything
     */
    bool is_copy_fn;
    /**
     * is this a clear function
     */
    bool is_post_move_fn;
    /**
     * is this a move function
     */
    bool is_move_fn;
    /**
     * is this a delete function
     */
    bool is_delete_fn;
    /**
     * the function overrides another present above in a struct or interface
     */
    bool is_unsafe;
    /**
     * is this function overriding a function
     */
    bool is_override;

    /**
     * has usage is set to true if function's pointer is taken or function is called
     */
    bool has_usage;

};

class FunctionDeclaration : public ASTNode, public FunctionTypeBody {
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
     * optional body
     */
    std::optional<Scope> body;
    /**
     * this index corresponds to number of iterations in llvm_data, for which function bodies
     * have been generated, so next time we should start at this index to generate bodies
     */
    int16_t bodies_gen_index = 0;
    /**
     * if this function an instantiation of a generic function, this field is set to the
     * generic decl, of which function this is instantiation of, which field helps us
     * register generic calls inside the current function
     */
    GenericFuncDecl* generic_parent = nullptr;
    /**
     * this is set by generic func decl only when this function is an instantiation of a generic function
     * this is the instantiation number of the generic function, this helps us identify this function in generic decl
     */
    int generic_instantiation = -1;

    /**
     * the llvm data
     */
#ifdef COMPILER_BUILD
    std::vector<llvm::Function*> llvm_data;
#endif

    /**
     * external data is stored in
     */
    FuncDeclAttributes attrs;

    /**
     * constructor
     */
    FunctionDeclaration(
            LocatedIdentifier identifier,
            BaseType* returnType,
            bool isVariadic,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal,
            bool signature_resolved = false,
            ASTNodeKind k = ASTNodeKind::FunctionDecl
    )  : ASTNode(k, parent_node, location), FunctionTypeBody(returnType, isVariadic, false, location, signature_resolved),
         identifier(identifier),
         attrs(specifier, false, false, 0, false, false, false, false, false, false, false, false, false, false, false) {
    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_compiler_decl() {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
    }

    inline void set_specifier_fast(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline void set_identifier(LocatedIdentifier id) {
        identifier = std::move(id);
    }

    inline const chem::string_view& name_view() {
        return identifier.identifier;
    }

    inline const chem::string_view name_view() const {
        return identifier.identifier;
    }

    inline const std::string name_str() const {
        return identifier.identifier.str();
    }

    inline uint8_t multi_func_index() {
        return attrs.multi_func_index;
    }

    inline void set_multi_func_index(uint8_t i) {
        attrs.multi_func_index = i;
    }

    inline bool is_extern() {
        return attrs.is_extern;
    }

    inline void set_extern(bool value) {
        attrs.is_extern = value;
    }

    inline bool is_cpp_mangle() {
        return attrs.is_cpp_mangle;
    }

    inline void set_cpp_mangle(bool value) {
        attrs.is_cpp_mangle = value;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_implicit() {
        return attrs.is_implicit;
    }

    inline void set_implicit(bool value) {
        attrs.is_implicit = value;
    }

    inline bool is_noReturn() {
        return attrs.is_noReturn;
    }

    inline void set_noReturn(bool value) {
        attrs.is_noReturn = value;
    }

    inline bool is_constructor_fn() {
        return attrs.is_constructor_fn;
    }

    inline void set_constructor_fn(bool value) {
        attrs.is_constructor_fn = value;
    }

    inline bool is_copy_fn() {
        return attrs.is_copy_fn;
    }

    inline void set_copy_fn(bool value) {
        attrs.is_copy_fn = value;
    }

    inline bool has_usage() {
        return attrs.has_usage;
    }

    inline void set_has_usage(bool usage) {
        attrs.has_usage = usage;
    }

    [[deprecated]]
    inline bool is_clear_fn() {
        return attrs.is_post_move_fn;
    }

    [[deprecated]]
    inline void set_clear_fn(bool value) {
        attrs.is_post_move_fn = value;
    }

    inline bool is_post_move_fn() {
        return attrs.is_post_move_fn;
    }

    inline void set_post_move_fn(bool value) {
        attrs.is_post_move_fn = value;
    }

    inline bool is_move_fn() {
        return attrs.is_move_fn;
    }

    inline void set_move_fn(bool value) {
        attrs.is_move_fn = value;
    }

    inline bool is_delete_fn() {
        return attrs.is_delete_fn;
    }

    inline void set_delete_fn(bool value) {
        attrs.is_delete_fn = value;
    }

    inline bool is_unsafe() {
        return attrs.is_unsafe;
    }

    inline void set_unsafe(bool value) {
        attrs.is_unsafe = value;
    }

    inline bool is_override() {
        return attrs.is_override;
    }

    inline void set_override(bool value) {
        attrs.is_override = value;
    }

    inline bool is_auto_called_func() {
        return is_delete_fn() || is_copy_fn() || is_post_move_fn() || is_move_fn() || is_constructor_fn();
    }

    /**
     * check if the function actually exits at runtime
     * comptime functions or functions which have no usage do not exist
     */
    inline bool exists_at_runtime() {
        return !is_comptime() && (has_usage() || is_exported_fast() || is_auto_called_func());
    }

    /**
     * this creates a shallow copy of the function, excluding the nodes in the body
     */
    FunctionDeclaration* shallow_copy(ASTAllocator& allocator) {
        const auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(
                identifier,
                returnType,
                isVariadic(),
                parent(),
                ASTNode::encoded_location(),
                specifier(),
                FunctionType::data.signature_resolved
        );
        FunctionTypeBody::shallow_copy_into(*decl, allocator, decl);
        if(body.has_value()) {
            decl->body.emplace(decl, body->encoded_location());
            body->shallow_copy_into(decl->body.value());
        }
        decl->attrs = attrs;
        return decl;
    }

    /**
     * this creates a shallow copy of the function, excluding the nodes in the body
     */
    FunctionDeclaration* copy(ASTAllocator &allocator) override {
        const auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(
            identifier,
            // return type is copied by the function type
            returnType,
            isVariadic(),
            parent(),
            ASTNode::encoded_location(),
            specifier(),
            FunctionType::data.signature_resolved
        );
        FunctionTypeBody::copy_into(*decl, allocator, decl);
        if(body.has_value()) {
            decl->body.emplace(decl, body->encoded_location());
            body->copy_into(decl->body.value(), allocator, decl);
        }
        decl->attrs = attrs;
        return decl;
    }

    bool is_exported() {
        return specifier() == AccessSpecifier::Public;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    std::string runtime_name_fast() {
        return ASTNode::parent() ? runtime_name_str() : runtime_name_no_parent_fast_str();
    }

    void runtime_name_no_parent_fast(std::ostream &stream);

    std::string runtime_name_no_parent_fast_str();

    void runtime_name_no_parent(std::ostream &stream) final {
        return runtime_name_no_parent_fast(stream);
    }

    void runtime_name(std::ostream &stream) final;


    LocatedIdentifier* get_func_name_id() final {
        return &identifier;
    }

    void make_destructor(ASTAllocator& allocator, ExtendableMembersContainerNode* def);

    void ensure_constructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, StructDefinition* def);

    void ensure_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def);

    void ensure_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def);

    void ensure_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def);

    void ensure_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def);

    using FunctionType::as_extension_func;

    /**
     * if there's no body then returns the location of function
     */
    SourceLocation body_location() {
        return (body.has_value()) ? body->encoded_location() : ASTNode::encoded_location();
    }

    /**
     * we return generic iteration if it already exists
     */
    int16_t register_call_with_existing(ASTDiagnoser& diagnoser, FunctionCall* call, BaseType* expected_type);

    void register_parent_iteration(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, int16_t parent_itr);

    int16_t get_parent_iteration();

    /**
     * a call notifies a function, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    int16_t register_call(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, FunctionCall* call, BaseType* expected_type);

    /**
     * get known function type, which is this
     */
    BaseType* known_type() final {
        return this;
    }

#ifdef COMPILER_BUILD

    void llvm_attributes(llvm::Function* func);

    llvm::Function*& get_llvm_data();

    inline llvm::Function* llvm_func() {
        return get_llvm_data();
    }

    llvm::Type *llvm_type(Codegen &gen) final;

    inline llvm::Value* llvm_load(Codegen& gen, SourceLocation location) final {
        return (llvm::Value*) get_llvm_data();
    }

    inline llvm::Value* llvm_pointer(Codegen &gen) final {
        return (llvm::Value*) get_llvm_data();
    }

    std::vector<llvm::Type *> param_types(Codegen &gen);

    llvm::FunctionType *create_llvm_func_type(Codegen &gen);

    /**
     * this function will take into account @cpp annotation
     */
    std::string runtime_name_fast(Codegen& gen);

    /**
     * get the known func or nullptr
     */
    llvm::Function* known_func();

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
    llvm::FunctionType *llvm_func_type(Codegen &gen);

    /**
     * given llvm data will be set for active iteration
     */
    void set_llvm_data(llvm::Function* func);

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
     * codegen constructor is called by function declaration itself
     * when a constructor's body is to be generated
     */
    void code_gen_constructor(Codegen& gen, StructDefinition* def);

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

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) override;

    void link_signature_no_scope(SymbolResolver& linker);

    void link_signature(SymbolResolver &linker) override;

    void redeclare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) override;

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
    void set_return(InterpretScope& func_scope, Value *value) final;

    FunctionDeclaration *as_function() final;

};