// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "ASTAny.h"
#include "Interpretable.h"
#include "Visitor.h"
#include "BaseTypeKind.h"
#include "ValueType.h"
#include "std/hybrid_ptr.h"
#include <iostream>

class SymbolResolver;

class FunctionParam;

class BaseType;

class LoopASTNode;

class EnumMember;

class CapturedVariable;

class ExtendableBase;

class BaseFunctionParam;

class AnnotableNode;

class MultiFunctionNode;

class VariablesContainer;

class GenericTypeParameter;

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode : public Interpretable, public ASTAny {
public:

    /**
     * default constructor
     */
    ASTNode() = default;

    /**
     * deleted copy constructor
     */
    ASTNode(const ASTNode& other) = delete;

    /**
     * default move constructor
     */
    ASTNode(ASTNode&& other) = default;

    /**
     * move assignment constructor
     */
    ASTNode& operator =(ASTNode &&other) noexcept = default;

    /**
     * declare something on the scope map
     * that must be retained in nested level scopes
     * for example top level functions can be called within functions
     */
    virtual void declare_top_level(SymbolResolver &linker) {
        // does nothing by default
    }

    /**
     * declares something on the scope map
     * or find something on the map to link yourself with it
     */
    virtual void declare_and_link(SymbolResolver &linker) {
        // does nothing by default
    }

    /**
     * this function provides a pointer to the parent ASTNode
     * a var init inside for loop, gets a pointer to the for loop
     */
    virtual ASTNode *parent() = 0;

    /**
     * this function provides a pointer to the parent ASTNode
     * a var init inside for loop, gets a pointer to the for loop
     */
    virtual void set_parent(ASTNode*) {
        throw std::runtime_error("set_parent called on base ast node");
    }

    /**
     * return a child ASTNode* at index, called by index operator
     * WARNING : index can be -1, if not known at compile time !
     */
    virtual ASTNode *child(int index) {
        return nullptr;
    }

    /**
     * return a child ASTNode* with name
     * called by access chain values like function call, on structs to get member function definitions
     */
    virtual ASTNode *child(const std::string &name) {
        return nullptr;
    }

    /**
     * same as child, only it returns the index of the child
     * so it can be used to create get element pointer instructions using llvm
     */
    virtual int child_index(const std::string &name) {
        return -1;
    }

    /**
     * any value held by this node, for example var init statement can hold an initializer
     */
    virtual Value* holding_value() {
        return nullptr;
    }

    /**
     * any type held by this node, for a value
     * for example var init can be initialized with a type instead of a value, and this type will be provided
     */
    virtual BaseType* holding_value_type() {
        return nullptr;
    }

    /**
     * This would return the representation of the node
     */
    std::string representation();

    /**
     * when this node is inside a namespace, what identifier can be used
     * to access this node, for example a function inside a namespace, returns it's name
     * so namespace::function_name can be used to access it
     */
    virtual std::string ns_node_identifier() {
        return "";
    }

    /**
     * return this as a generic type parameter if its one
     */
    virtual GenericTypeParameter* as_generic_type_param() {
        return nullptr;
    }

    /**
     * get the extendable members container, if this node has one
     */
    virtual ExtendableBase* as_extendable_members_container() {
        return nullptr;
    }

    /**
     * return this as an annotable node
     */
    virtual AnnotableNode* as_annotable_node() {
        return nullptr;
    }

    /**
     * return this as a multi function node
     */
    virtual MultiFunctionNode* as_multi_func_node() {
        return nullptr;
    }

    /**
     * as enum member
     */
    virtual EnumDeclaration* as_enum_decl() {
        return nullptr;
    }

    /**
     * as enum member
     */
    virtual EnumMember* as_enum_member() {
        return nullptr;
    }

    /**
     * return if this is a base function paam
     */
    virtual BaseFunctionParam* as_base_func_param() {
        return nullptr;
    }

    /**
     * get as extension function
     */
    virtual ExtensionFunction* as_extension_func() {
        return nullptr;
    }

    /**
     * return if this is a parameter
     * @return
     */
    virtual FunctionParam *as_func_param() {
        return nullptr;
    }

    /**
     * return if this is a function decl
     * @return
     */
    virtual FunctionDeclaration *as_function() {
        return nullptr;
    }

    /**
     * return if this is a struct member
     */
    virtual StructMember *as_struct_member() {
        return nullptr;
    }

    /**
     * return if this is an unnamed union
     */
    virtual UnnamedUnion *as_unnamed_union() {
        return nullptr;
    }

    /**
     * return if this is an unnamed struct
     */
    virtual UnnamedStruct *as_unnamed_struct() {
        return nullptr;
    }

    /**
     * return if this is a typealias statement
     */
    virtual TypealiasStatement *as_typealias() {
        return nullptr;
    }

    /**
     * return if this is a captured variable
     */
    virtual CapturedVariable *as_captured_var() {
        return nullptr;
    }

    /**
     * return if this is a return statement
     */
    virtual ReturnStatement *as_return() {
        return nullptr;
    }

    /**
     * return if this is a loop ast node
     */
    virtual LoopASTNode *as_loop_ast() {
        return nullptr;
    }

    /**
     * as interface definition
     */
    virtual InterfaceDefinition *as_interface_def() {
        return nullptr;
    }

    /**
     * as namespace
     */
    virtual Namespace* as_namespace() {
        return nullptr;
    }

    /**
     * return if this is a variables container
     */
    virtual VariablesContainer *as_variables_container() {
        return nullptr;
    }

    /**
     * return if this is a struct definition
     */
    virtual StructDefinition *as_struct_def() {
        return nullptr;
    }

    /**
     * return if this is a implementation def
     */
    virtual ImplDefinition* as_impl_def() {
        return nullptr;
    }

    /**
     * return if this is a struct definition
     */
    virtual UnionDef *as_union_def() {
        return nullptr;
    }

    /**
     * return if this is a var init statement
     */
    virtual VarInitStatement *as_var_init() {
        return nullptr;
    }

    /**
     * return assignment statement if it is one
     */
    virtual AssignStatement* as_assignment() {
        return nullptr;
    }

    /**
     * get the byte size, of this type
     */
    virtual uint64_t byte_size(bool is64Bit);

    /**
     * accept the visitor
     */
    virtual void accept(Visitor *visitor) = 0;

    /**
     * get the type from the ASTNode
     * this type can represent the type of value, type of parameter etc.
     */
    virtual std::unique_ptr<BaseType> create_value_type() {
        throw std::runtime_error("create_value_type called on bare ASTNode, with representation" + representation());
    }

    /**
     * this returns a hybrid pointer, which decreases the number of allocations, because
     * type of value may be known by the value
     */
    virtual hybrid_ptr<BaseType> get_value_type() {
        throw std::runtime_error("create_value_type called on bare ASTNode, with representation" + representation());
    }

#ifdef COMPILER_BUILD

    /**
     * returns a llvm pointer
     */
    virtual llvm::Value *llvm_pointer(Codegen &gen) {
        throw std::runtime_error("llvm_pointer called on bare ASTNode, with representation" + representation());
    };

    /**
     * return a llvm func type, so that this ASTNode can be called
     */
    virtual llvm::Type* llvm_param_type(Codegen &gen) {
        return llvm_type(gen);
    }

    /**
     * return a llvm func type, so that this ASTNode can be called
     */
    virtual llvm::FunctionType* llvm_func_type(Codegen &gen) {
        return (llvm::FunctionType*) llvm_type(gen);
    }

    /**
     * will call destructor on allocated value, if required
     */
    virtual void llvm_destruct(Codegen& gen, llvm::Value* allocaInst) {
        // no destruction required
    }

    /**
     * provides llvm_elem_type, which is the child type for example elem type of an array value
     * @param gen
     * @return
     */
    virtual llvm::Type *llvm_elem_type(Codegen &gen) {
        throw std::runtime_error("llvm_elem_type called on bare ASTNode, with representation" + representation());
    };

    /**
     * this can be overridden if node intends to declare itself before generating code for it
     * functions generate code for just prototype and an empty entry block
     * when code_gen is called, functions generate code for their bodies
     * this means that functions that are declared below the current function can be called by the calls inside the bodies of functions above
     */
    virtual void code_gen_declare(Codegen &gen) {
        // node can declare itself
    }

    /**
     * code_gen function that generates llvm Value
     */
    virtual void code_gen(Codegen &gen) {
        throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation());
    }

    /**
     * instead of calling code_gen, this function can be called, to provide more information
     * so that code generation can be better
     * for example if statement needs to know whether this is the last node, so that it can
     * generate end block optionally based on whether there's more code after if statement
     * in current scope
     */
    virtual void code_gen(Codegen &gen, Scope* scope, unsigned int index) {
        code_gen(gen);
    }

    /**
     * if a node can destroy itself, for example a variable declaration which contains a struct value
     * where struct has a destructor, here -> variable init will ask value to destruct itself
     * which will then ask struct def to destruct the value, and then struct def will create a call
     * to destructor
     * @param returnValue is the value being returned, which mustn't be destructed
     */
    virtual void code_gen_destruct(Codegen &gen, Value* returnValue) {
        // does nothing by default
    }

    /**
     * add child index in llvm indexes vector
     */
    virtual bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
#ifdef DEBUG
        std::cerr << "add_child_index called on base ASTNode, representation : " << representation();
#endif
        throw std::runtime_error("add_child_index called on a ASTNode");
    }

    /**
     * loads the value of the given ASTNode
     * this is called by variable identifier, on linked nodes (var init, function parameters)
     */
    virtual llvm::Value *llvm_load(Codegen &gen) {
#ifdef DEBUG
        std::cerr << "llvm_load called on base ASTNode, representation : " << representation();
#endif
        throw std::runtime_error("llvm_load called on a ASTNode");
    }

    /**
     * loads the value of the given ASTNode for returning
     */
    virtual llvm::Value *llvm_ret_load(Codegen &gen, ReturnStatement* returnStmt) {
        return llvm_load(gen);
    }

#endif

    /**
     * get the type kind represented by this node
     */
    virtual BaseTypeKind type_kind() const {
        return BaseTypeKind::Unknown;
    }

    /**
     * get the value type represented by this node
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    }

    /**
     * virtual destructor for the ASTNode
     */
    virtual ~ASTNode();

};