// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Interpretable.h"
#include "Visitor.h"
#include "BaseTypeKind.h"
#include "ValueType.h"

class SymbolResolver;

#ifdef COMPILER_BUILD

class Codegen;

#include "compiler/llvmfwd.h"

#endif

#ifdef DEBUG
#include <iostream>
#endif

class FunctionParam;

class BaseType;

class LoopASTNode;

class EnumMember;

class CapturedVariable;

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode : public Interpretable {
public:

    /**
     * the position of of the ASTNode is its position in the current function
     * this calculated when interpreting ASTNodes
     *
     * var i = 0 // 0
     * if(i == 0) { // 1
     *    i = 1; // 2
     * }
     */
    unsigned int position = 0;

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
     * This would return the representation of the node
     * @return
     */
    virtual std::string representation() const = 0;

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
#ifdef DEBUG
        std::cerr << "as_interface_def called on ASTNode" << std::endl;
#endif
        return nullptr;
    }

    /**
     * return if this is a struct definition
     */
    virtual StructDefinition *as_struct_def() {
#ifdef DEBUG
        std::cerr << "as_struct_def called on ASTNode" << std::endl;
#endif
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
     * accept the visitor
     */
    virtual void accept(Visitor *visitor) = 0;

    /**
     * This supposed to be overridden by ASTNodes that put themselves
     * on global value map, to clean up when the current interpret scope ends
     * this method can be overridden
     */
    virtual void interpret_scope_ends(InterpretScope &scope) {

    }

    /**
     * get the type from the ASTNode
     * this type can represent the type of value, type of parameter etc.
     */
    virtual std::unique_ptr<BaseType> create_value_type() {
        throw std::runtime_error("create_value_type called on bare ASTNode, with representation" + representation());
    }

#ifdef COMPILER_BUILD

    /**
     * returns a llvm pointer
     * @param gen
     * @return
     */
    virtual llvm::Value *llvm_pointer(Codegen &gen) {
        throw std::runtime_error("llvm_pointer called on bare ASTNode, with representation" + representation());
    };

    /**
     * provides llvm_type if this statement declares a type
     * @param gen
     * @return
     */
    virtual llvm::Type *llvm_type(Codegen &gen) {
        throw std::runtime_error("llvm_type called on bare ASTNode, with representation" + representation());
    };

    /**
     * return a llvm func type, so that this ASTNode can be called
     */
    virtual llvm::FunctionType* llvm_func_type(Codegen &gen) {
        return (llvm::FunctionType*) llvm_type(gen);
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
     * code_gen function that generates llvm Value
     * @return
     */
    virtual void code_gen(Codegen &gen) {
        throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation());
    }

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
     * instead of calling code_gen, this function can be called, to provide more information
     * so that code generation can be better
     * for example if statement needs to know whether this is the last node, so that it can
     * generate end block optionally based on whether there's more code after if statement
     * in current scope
     */
    virtual void code_gen(Codegen &gen, std::vector<std::unique_ptr<ASTNode>>& nodes, unsigned int index) {
        code_gen(gen);
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
    virtual ~ASTNode() = default;

};