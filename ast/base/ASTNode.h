// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include "ASTAny.h"
#include "ASTNodeKind.h"
#include "AccessSpecifier.h"
#include "ASTAllocator.h"
#include "DebugCast.h"
#include "std/chem_string.h"

class SymbolResolver;

class LocatedIdentifier;

class FunctionParam;

class BaseType;

class LoopASTNode;

class EnumMember;

class CapturedVariable;

class VariantCaseVariable;

class ExtendableBase;

class ExtendableMembersContainerNode;

class AnnotableNode;

class MultiFunctionNode;

class VariablesContainer;

class MembersContainer;

class GenericTypeParameter;

class BaseDefMember;

/**
 * Base class for ast node
 */
class ASTNode : public ASTAny {
private:

    /**
     * kind is stored
     */
    ASTNodeKind const _kind;

    /**
     * the parent of this ast node
     */
    ASTNode* _parent;

    /**
     * encoded source location
     */
    SourceLocation _location;

public:

    /**
     * default constructor
     */
    inline explicit constexpr ASTNode(
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation loc
    ) noexcept : _kind(k), _parent(parent), _location(loc) {
        // does nothing
    }

    /**
     * deleted copy constructor
     */
    ASTNode(const ASTNode& other) = delete;

    /**
     * default move constructor
     */
    ASTNode(ASTNode&& other) = default;

    /**
     * any kind of 'node' is returned
     */
    ASTAnyKind any_kind() final {
        return ASTAnyKind::Node;
    }

    /**
     * get the kind of ast node
     */
    inline ASTNodeKind kind() const noexcept {
        return _kind;
    }

    /**
     * get the encoded location
     */
    inline SourceLocation encoded_location() const noexcept {
        return _location;
    }

    /**
     * this sets the new location for this node
     */
    inline void set_encoded_location(SourceLocation new_loc) noexcept {
        _location = new_loc;
    }

    /**
     * this function provides a pointer to the parent ASTNode
     * a var init inside for loop, gets a pointer to the for loop
     */
    inline ASTNode* parent() const noexcept {
        return _parent;
    }

    /**
     * update the parent node of this node
     */
    inline void set_parent(ASTNode* new_parent) noexcept {
        _parent = new_parent;
    }

    /**
     * check if this node is in file scope
     */
    inline bool is_directly_in_file_scope() {
        return parent() && parent()->kind() == ASTNodeKind::FileScope;
    }

    /**
     * is top level node
     * 1 - a node is inside the file
     * 2 - a node is inside a namespace
     */
    bool is_top_level();

    /**
     * declare something on the scope map
     * that must be retained in nested level scopes
     * for example top level functions can be called within functions
     */
    virtual void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
        // does nothing by default
    }

    /**
     * link signature is called in between declare_top_level and declare_and_link calls
     * we link the signature in between, this function is currently only relevant to functions
     * and probably will be needed by other declarations in the future
     * because when declare_and_link is called, function bodies can contain calls
     * to functions that are below it, whose signature hasn't been resolved which causes bugs
     */
    virtual void link_signature(SymbolResolver &linker) {
        // does nothing by default
    }

    /**
     * declares something on the scope map
     * or find something on the map to link yourself with it
     */
    virtual void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
        // does nothing by default
    }

    /**
     * create a deep copy of the node
     */
    virtual ASTNode* copy(ASTAllocator& allocator) {
        return nullptr;
    }

    /**
     * This get's the root parent of the current node
     */
    ASTNode* root_parent();

    /**
     * get access specifier for this node
     */
    AccessSpecifier specifier();

    /**
     * will set the specifier to given, returns true if set
     */
    bool set_specifier(AccessSpecifier spec);

    /**
     * is this node's being exported outside this module
     */
    bool is_exported();

    /**
     * makes node compile time
     * @return true if successful otherwise false
     */
    bool set_comptime(bool value);

    /**
     * makes the node deprecated
     * @return true if successful otherwise false
     */
    bool set_deprecated(bool value);

    /**
     * makes the struct | union | variant anonymous
     * @return true if successful otherwise false
     */
    bool set_anonymous(bool value);

    /**
     * a struct/union/variant can be makrked copy to force compiler to shallow copy everywhere
     */
    bool is_shallow_copyable();

    /**
     * a struct/union/variant can be makrked copy to force compiler to shallow copy everywhere
     */
    void set_shallow_copyable(bool value);

    /**
     * type is only returned if the value is guaranteed to be stored in a storage location
     * function param is not a storage location, however var decl, struct member, variant member param qualify
     */
    BaseType* get_stored_value_type(ASTAllocator& allocator, ASTNodeKind k);

    /**
     * is this node storing a pointer, stored pointer must be loaded
     * before use
     */
    bool is_stored_ptr_or_ref(ASTAllocator& allocator, ASTNodeKind k);

    /**
     * this checks if it's any pointer, like in function params
     * which doesn't have a backing storage location
     */
    bool is_ptr_or_ref(ASTAllocator& allocator, ASTNodeKind k);

    /**
     * a helper function, check if this is a stored pointer
     */
    bool is_stored_ptr_or_ref(ASTAllocator& allocator) {
        return is_stored_ptr_or_ref(allocator, kind());
    }

    /**
     * check if it is a stored reference
     */
    bool is_stored_ref(ASTAllocator& allocator);

    /**
     * check if this is a reference
     */
    bool is_ref(ASTAllocator& allocator);

    /**
     * check if the given type is movable
     */
    bool requires_moving(ASTNodeKind k);

    /**
     * return a child ASTNode* with name
     * called by access chain values like function call, on structs to get member function definitions
     */
    virtual ASTNode *child(const chem::string_view &name) {
        return nullptr;
    }

    /**
     * any value held by this node, for example var init statement can hold an initializer
     */
    virtual Value* holding_value() {
        return nullptr;
    }

    /**
     * The type for this node, this could be type of value of var init
     */
    virtual BaseType* known_type() {
        return nullptr;
    }

    /**
     * get members container (if this node is one, or if linked with one)
     */
    MembersContainer* get_members_container();

    /**
     * This would return the representation of the node
     */
    std::string representation();

    /**
     * get located id
     * @return id if the node declares a identifier otherwise null pointer
     */
    LocatedIdentifier* get_located_id();

    /**
     * get the extendable members container, if this node has one
     */
    virtual ExtendableBase* as_extendable_members_container() {
        return nullptr;
    }

    /**
     * get as extendable members container node
     */
    virtual ExtendableMembersContainerNode* as_extendable_members_container_node() {
        return nullptr;
    }

    /**
     * return if this is a variables container
     */
    virtual VariablesContainer *as_variables_container() {
        return nullptr;
    }

    /**
     * get the byte size, of this type
     */
    virtual uint64_t byte_size(bool is64Bit);

    /**
     * this should only be called, where the type is required during symbol resolution
     * since types are determined during or after symbol resolution completes
     * this allows to get the partially determined type, which could include types
     * that are linked with generic type parameters
     */
    BaseType* known_type_SymRes(ASTAllocator& allocator);

#ifdef COMPILER_BUILD

    /**
     * returns a llvm pointer
     */
    virtual llvm::Value *llvm_pointer(Codegen &gen);

    /**
     * return a llvm func type, so that this ASTNode can be called
     */
    virtual llvm::Type* llvm_param_type(Codegen &gen) {
        return llvm_type(gen);
    }

    /**
     * will call destructor on allocated value, if required
     */
    void llvm_destruct(Codegen& gen, llvm::Value* allocaInst, SourceLocation location);

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
     * this is called on nodes that are being imported into another module
     * for the first time, so this basically declares nodes (functions) like
     * you would declare functions present in external modules (libraries)
     * that are linked by the linker in C or C++
     */
    virtual void code_gen_external_declare(Codegen &gen) {
        // node can declare it self
    }

    /**
     * code_gen function that generates llvm Value
     */
    virtual void code_gen(Codegen &gen);

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
    void code_gen_destruct(Codegen &gen, Value* returnValue, SourceLocation location);

    /**
     * add child index in llvm indexes vector
     */
    virtual bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name);

    /**
     * loads the value of the given ASTNode
     * this is called by variable identifier, on linked nodes (var init, function parameters)
     */
    virtual llvm::Value* llvm_load(Codegen &gen, SourceLocation location);

#endif

    /**
     * virtual destructor for the ASTNode
     */
    virtual ~ASTNode();

    //---------------------------------------------
    // Helper is methods
    //---------------------------------------------

    static inline constexpr bool isInitBlock(ASTNodeKind k) {
        return k == ASTNodeKind::InitBlock;
    }

    static inline constexpr bool isValueWrapperNode(ASTNodeKind k) {
        return k == ASTNodeKind::ValueWrapper;
    }

    static inline constexpr bool isAssignmentStmt(ASTNodeKind k) {
        return k == ASTNodeKind::AssignmentStmt;
    }

    static inline constexpr bool isBreakStmt(ASTNodeKind k) {
        return k == ASTNodeKind::BreakStmt;
    }

    static inline constexpr bool isContinueStmt(ASTNodeKind k) {
        return k == ASTNodeKind::ContinueStmt;
    }

    static inline constexpr bool isDeleteStmt(ASTNodeKind k) {
        return k == ASTNodeKind::DeleteStmt;
    }

    static inline constexpr bool isImportStmt(ASTNodeKind k) {
        return k == ASTNodeKind::ImportStmt;
    }

    static inline constexpr bool isReturnStmt(ASTNodeKind k) {
        return k == ASTNodeKind::ReturnStmt;
    }

    static inline constexpr bool isSwitchStmt(ASTNodeKind k) {
        return k == ASTNodeKind::SwitchStmt;
    }

    static inline constexpr bool isThrowStmt(ASTNodeKind k) {
        return k == ASTNodeKind::ThrowStmt;
    }

    static inline constexpr bool isTypealiasStmt(ASTNodeKind k) {
        return k == ASTNodeKind::TypealiasStmt;
    }

    static inline constexpr bool isUsingStmt(ASTNodeKind k) {
        return k == ASTNodeKind::UsingStmt;
    }

    static inline constexpr bool isVarInitStmt(ASTNodeKind k) {
        return k == ASTNodeKind::VarInitStmt;
    }

    static inline constexpr bool isWhileLoopStmt(ASTNodeKind k) {
        return k == ASTNodeKind::WhileLoopStmt;
    }

    static inline constexpr bool isDoWhileLoopStmt(ASTNodeKind k) {
        return k == ASTNodeKind::DoWhileLoopStmt;
    }

    static inline constexpr bool isForLoopStmt(ASTNodeKind k) {
        return k == ASTNodeKind::ForLoopStmt;
    }

    static inline constexpr bool isIfStmt(ASTNodeKind k) {
        return k == ASTNodeKind::IfStmt;
    }

    static inline constexpr bool isTryStmt(ASTNodeKind k) {
        return k == ASTNodeKind::TryStmt;
    }

    static inline constexpr bool isEnumDecl(ASTNodeKind k) {
        return k == ASTNodeKind::EnumDecl;
    }

    static inline constexpr bool isEnumMember(ASTNodeKind k) {
        return k == ASTNodeKind::EnumMember;
    }

    static inline constexpr bool isFunctionDecl(ASTNodeKind k) {
        return k == ASTNodeKind::FunctionDecl;
    }

    static inline constexpr bool isMultiFunctionNode(ASTNodeKind k) {
        return k == ASTNodeKind::MultiFunctionNode;
    }

    static inline constexpr bool isImplDecl(ASTNodeKind k) {
        return k == ASTNodeKind::ImplDecl;
    }

    static inline constexpr bool isInterfaceDecl(ASTNodeKind k) {
        return k == ASTNodeKind::InterfaceDecl;
    }

    static inline constexpr bool isStructDecl(ASTNodeKind k) {
        return k == ASTNodeKind::StructDecl;
    }

    static inline constexpr bool isStructMember(ASTNodeKind k) {
        return k == ASTNodeKind::StructMember;
    }

    static inline constexpr bool isNamespaceDecl(ASTNodeKind k) {
        return k == ASTNodeKind::NamespaceDecl;
    }

    static inline constexpr bool isUnionDecl(ASTNodeKind k) {
        return k == ASTNodeKind::UnionDecl;
    }

    static inline constexpr bool isVariantDecl(ASTNodeKind k) {
        return k == ASTNodeKind::VariantDecl;
    }

    static inline constexpr bool isVariantMember(ASTNodeKind k) {
        return k == ASTNodeKind::VariantMember;
    }

    static inline constexpr bool isUnnamedStruct(ASTNodeKind k) {
        return k == ASTNodeKind::UnnamedStruct;
    }

    static inline constexpr bool isUnnamedUnion(ASTNodeKind k) {
        return k == ASTNodeKind::UnnamedUnion;
    }

    static inline constexpr bool isScope(ASTNodeKind k) {
        return k == ASTNodeKind::Scope;
    }

    static inline constexpr bool isFunctionParam(ASTNodeKind k) {
        return k == ASTNodeKind::FunctionParam;
    }

    static inline constexpr bool isGenericTypeParam(ASTNodeKind k) {
        return k == ASTNodeKind::GenericTypeParam;
    }

    static inline constexpr bool isVariantMemberParam(ASTNodeKind k) {
        return k == ASTNodeKind::VariantMemberParam;
    }

    static inline constexpr bool isCapturedVariable(ASTNodeKind k) {
        return k == ASTNodeKind::CapturedVariable;
    }

    static inline constexpr bool isVariantCaseVariable(ASTNodeKind k) {
        return k == ASTNodeKind::VariantCaseVariable;
    }

    static inline constexpr bool isBaseFuncParam(ASTNodeKind k) {
        return k == ASTNodeKind::FunctionParam;
    }

    static inline constexpr bool isLoopASTNode(ASTNodeKind k) {
        return k == ASTNodeKind::WhileLoopStmt || k == ASTNodeKind::DoWhileLoopStmt || k == ASTNodeKind::ForLoopStmt || k == ASTNodeKind::LoopBlock;
    }

    static inline constexpr bool isMembersContainer(ASTNodeKind k) {
        return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::InterfaceDecl || k == ASTNodeKind::ImplDecl;
    }

    static inline constexpr bool isAnyStructMember(ASTNodeKind k) {
        return k == ASTNodeKind::StructMember || k == ASTNodeKind::UnnamedStruct || k == ASTNodeKind::UnnamedUnion;
    }

    static inline constexpr bool isBaseDefMember(ASTNodeKind k) {
        return isAnyStructMember(k) || k == ASTNodeKind::VariantMember;
    }

    static inline constexpr bool isStoredStructType(ASTNodeKind k) {
        return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::VariantMember || k == ASTNodeKind::InterfaceDecl || k == ASTNodeKind::UnnamedStruct || k == ASTNodeKind::UnnamedUnion;
    }

    static inline constexpr bool isStoredStructDecl(ASTNodeKind k) {
        return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::InterfaceDecl;
    }

    static inline constexpr bool isAnnotableNode(ASTNodeKind k) {
        return k == ASTNodeKind::UsingStmt || k == ASTNodeKind::VarInitStmt || k == ASTNodeKind::NamespaceDecl || isBaseDefMember(k) || isFunctionDecl(k) || isMembersContainer(k);
    }

    //---------------------------------------------
    // Helper as (safe) methods
    //---------------------------------------------

    /**
     * return as init block safely
     */
    inline InitBlock* as_init_block() {
        return isInitBlock(kind()) ? (InitBlock*) this : nullptr;
    }

    /**
     * get as value wrapper node safely
     */
    inline ValueWrapperNode* as_value_wrapper() {
        return isValueWrapperNode(kind()) ? (ValueWrapperNode*) this : nullptr;
    }

    /**
      * return this as an annotable node
      */
    inline AnnotableNode* as_annotable_node() {
        return isAnnotableNode(kind()) ? (AnnotableNode*) this : nullptr;
    }

    /**
     * return if this is definition member
     */
    inline BaseDefMember* as_base_def_member() {
        return isBaseDefMember(kind()) ? (BaseDefMember*) this : nullptr;
    }

    /**
     * get a members container
     */
    inline MembersContainer* as_members_container() {
        return isMembersContainer(kind()) ? (MembersContainer*) this : nullptr;
    }

    /**
     * return if this is a loop ast node
     */
    inline LoopASTNode *as_loop_ast() {
        return isLoopASTNode(kind()) ? (LoopASTNode*) this : nullptr;
    }

    /**
     * return if this is a scope
     */
    inline Scope *as_scope() {
        return isScope(kind()) ? (Scope*) this : nullptr;
    }

    /**
     * return this as a generic type parameter if its one
     */
    inline GenericTypeParameter* as_generic_type_param() {
        return isGenericTypeParam(kind()) ? (GenericTypeParameter*) this : nullptr;
    }

    /**
     * return this as a multi function node
     */
    inline MultiFunctionNode* as_multi_func_node() {
        return isMultiFunctionNode(kind()) ? (MultiFunctionNode*) this : nullptr;
    }

    /**
     * as enum member
     */
    inline EnumDeclaration* as_enum_decl() {
        return isEnumDecl(kind()) ? (EnumDeclaration*) this : nullptr;
    }

    /**
     * as enum member
     */
    inline EnumMember* as_enum_member() {
        return isEnumMember(kind()) ? (EnumMember*) this : nullptr;
    }

    /**
     * return if this is a parameter
     */
    inline FunctionParam *as_func_param() {
        return isFunctionParam(kind()) ? (FunctionParam*) this : nullptr;
    }

    /**
     * return if this is a function decl
     * @return
     */
    inline FunctionDeclaration *as_function() {
        return isFunctionDecl(kind()) ? (FunctionDeclaration*) this : nullptr;
    }

    /**
     * return if this is a struct member
     */
    inline StructMember *as_struct_member() {
        return isStructMember(kind()) ? (StructMember*) this : nullptr;
    }

    /**
     * return if this is an unnamed union
     */
    inline UnnamedUnion *as_unnamed_union() {
        return isUnnamedUnion(kind()) ? (UnnamedUnion*) this : nullptr;
    }

    /**
     * return if this is an unnamed struct
     */
    inline UnnamedStruct *as_unnamed_struct() {
        return isUnnamedStruct(kind()) ? (UnnamedStruct*) this : nullptr;
    }

    /**
     * return if this is a typealias statement
     */
    inline TypealiasStatement *as_typealias() {
        return isTypealiasStmt(kind()) ? (TypealiasStatement*) this : nullptr;
    }

    /**
     * return if this is a captured variable
     */
    inline CapturedVariable *as_captured_var() {
        return isCapturedVariable(kind()) ? (CapturedVariable*) this : nullptr;
    }

    /**
     * return if this is a return statement
     */
    inline ReturnStatement *as_return() {
        return isReturnStmt(kind()) ? (ReturnStatement*) this : nullptr;
    }

    /**
     * return if this is a using statement
     */
    inline UsingStmt* as_using_stmt() {
        return isUsingStmt(kind()) ? (UsingStmt*) this : nullptr;
    }

    /**
     * as interface definition
     */
    inline InterfaceDefinition *as_interface_def() {
        return isInterfaceDecl(kind()) ? (InterfaceDefinition*) this : nullptr;
    }

    /**
     * as namespace
     */
    inline Namespace* as_namespace() {
        return isNamespaceDecl(kind()) ? (Namespace*) this : nullptr;
    }

    /**
     * return if this is a struct definition
     */
    inline StructDefinition *as_struct_def() {
        return isStructDecl(kind()) ? (StructDefinition*) this : nullptr;
    }

    /**
     * return if this is a implementation def
     */
    inline ImplDefinition* as_impl_def() {
        return isImplDecl(kind()) ? (ImplDefinition*) this : nullptr;
    }

    /**
     * return if this is a struct definition
     */
    inline UnionDef *as_union_def() {
        return isUnionDecl(kind()) ? (UnionDef*) this : nullptr;
    }

    /**
     * return if this is a var init statement
     */
    inline VarInitStatement *as_var_init() {
        return isVarInitStmt(kind()) ? (VarInitStatement*) this : nullptr;
    }

    /**
     * return if this is a variant member
     */
    inline VariantMember* as_variant_member() {
        return isVariantMember(kind()) ? (VariantMember*) this : nullptr;
    }

    /**
     * return if this is a variant definition
     */
    inline VariantDefinition* as_variant_def() {
        return isVariantDecl(kind()) ? (VariantDefinition*) this : nullptr;
    }

    /**
     * return if this is a variant case variable
     */
    inline VariantCaseVariable* as_variant_case_var() {
        return isVariantCaseVariable(kind()) ? (VariantCaseVariable*) this : nullptr;
    }

    /**
     * return assignment statement if it is one
     */
    inline AssignStatement* as_assignment() {
        return isAssignmentStmt(kind()) ? (AssignStatement*) this : nullptr;
    }

    //---------------------------------------------
    // Helper as (unsafe) methods
    //---------------------------------------------

    /**
     * as value wrapper node unsafe
     */
    inline ValueWrapperNode* as_value_wrapper_unsafe() {
        CHECK_CAST(ASTNodeKind::ValueWrapper);
        return (ValueWrapperNode*) this;
    }

    /**
     * as value node unsafe
     */
    inline ValueNode* as_value_node_unsafe() {
        CHECK_CAST(ASTNodeKind::ValueNode);
        return (ValueNode*) this;
    }

    /**
     * get as alias stmt
     */
    inline AliasStmt* as_alias_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::AliasStmt);
        return (AliasStmt*) this;
    }

    /**
     * as loop node unsafe
     */
    inline LoopASTNode* as_loop_node_unsafe() {
        CHECK_COND(kind() == ASTNodeKind::WhileLoopStmt || kind() == ASTNodeKind::DoWhileLoopStmt || kind() == ASTNodeKind::ForLoopStmt || kind() == ASTNodeKind::LoopBlock);
        return (LoopASTNode*) this;
    }

    /**
     * get as for loop
     */
    inline ForLoop* as_for_loop_unsafe() {
        CHECK_CAST(ASTNodeKind::ForLoopStmt);
        return (ForLoop*) this;
    }

    /**
     * get as while loop
     */
    inline WhileLoop* as_while_loop_unsafe() {
        CHECK_CAST(ASTNodeKind::WhileLoopStmt);
        return (WhileLoop*) this;
    }

    /**
     * get as do while loop
     */
    inline DoWhileLoop* as_do_while_loop_unsafe() {
        CHECK_CAST(ASTNodeKind::DoWhileLoopStmt);
        return (DoWhileLoop*) this;
    }

    /**
     * as break stmt unsafe
     */
    inline BreakStatement* as_break_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::BreakStmt);
        return (BreakStatement*) this;
    }

    /**
     * as break stmt unsafe
     */
    inline ContinueStatement* as_continue_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::ContinueStmt);
        return (ContinueStatement*) this;
    }

    /**
     * get as if statement unsafely
     */
    inline IfStatement* as_if_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::IfStmt);
        return (IfStatement*) this;
    }

    /**
     * get as switch stmt unsafely
     */
    inline SwitchStatement* as_switch_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::SwitchStmt);
        return (SwitchStatement*) this;
    }

    /**
     * return if this is definition member
     */
    inline BaseDefMember* as_base_def_member_unsafe() {
        CHECK_COND(kind() == ASTNodeKind::StructMember || kind() == ASTNodeKind::UnnamedStruct || kind() == ASTNodeKind::UnnamedUnion || kind() == ASTNodeKind::VariantMember);
        return (BaseDefMember*) this;
    }

    /**
     * get a members container
     */
    inline MembersContainer* as_members_container_unsafe() {
        CHECK_COND(kind() == ASTNodeKind::StructDecl || kind() == ASTNodeKind::UnionDecl || kind() == ASTNodeKind::VariantDecl || kind() == ASTNodeKind::InterfaceDecl || kind() == ASTNodeKind::ImplDecl);
        return (MembersContainer*) this;
    }

    /**
     * get a extendable members container
     */
    inline ExtendableMembersContainerNode* as_extendable_members_container_unsafe() {
        CHECK_COND(kind() == ASTNodeKind::StructDecl || kind() == ASTNodeKind::UnionDecl || kind() == ASTNodeKind::VariantDecl || kind() == ASTNodeKind::InterfaceDecl);
        return (ExtendableMembersContainerNode*) this;
    }

    /**
     * return if this is a scope
     */
    inline Scope *as_scope_unsafe() {
        CHECK_CAST(ASTNodeKind::Scope);
        return (Scope*) this;
    }

    /**
     * return if this is a file scope
     */
    inline FileScope* as_file_scope_unsafe() {
        CHECK_CAST(ASTNodeKind::FileScope);
        return (FileScope*) this;
    }

    /**
     * return if this is a module scope
     */
    inline ModuleScope* as_module_scope_unsafe() {
        CHECK_CAST(ASTNodeKind::ModuleScope);
        return (ModuleScope*) this;
    }

    /**
     * return this as a generic type parameter if its one
     */
    inline GenericTypeParameter* as_generic_type_param_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericTypeParam);
        return (GenericTypeParameter*) this;
    }

    /**
     * return this as a multi function node
     */
    inline MultiFunctionNode* as_multi_func_node_unsafe() {
        CHECK_CAST(ASTNodeKind::MultiFunctionNode);
        return (MultiFunctionNode*) this;
    }

    /**
     * as enum member
     */
    inline EnumDeclaration* as_enum_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::EnumDecl);
        return (EnumDeclaration*) this;
    }

    /**
     * as enum member
     */
    inline EnumMember* as_enum_member_unsafe() {
        CHECK_CAST(ASTNodeKind::EnumMember);
        return (EnumMember*) this;
    }

    /**
     * return if this is a parameter
     */
    inline FunctionParam *as_func_param_unsafe() {
        CHECK_CAST(ASTNodeKind::FunctionParam);
        return (FunctionParam*) this;
    }

    /**
     * return if this is a function decl
     */
    inline FunctionDeclaration *as_function_unsafe() {
        CHECK_CAST(ASTNodeKind::FunctionDecl);
        return (FunctionDeclaration*) this;
    }

    /**
     * return if this is a generic func decl
     */
    inline GenericFuncDecl* as_gen_func_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericFuncDecl);
        return (GenericFuncDecl*) this;
    }

    /**
     * return if this is a struct member
     */
    inline StructMember *as_struct_member_unsafe() {
        CHECK_CAST(ASTNodeKind::StructMember);
        return (StructMember*) this;
    }

    /**
     * return if this is an unnamed union
     */
    inline UnnamedUnion *as_unnamed_union_unsafe() {
        CHECK_CAST(ASTNodeKind::UnnamedUnion);
        return (UnnamedUnion*) this;
    }

    /**
     * return if this is an unnamed struct
     */
    inline UnnamedStruct *as_unnamed_struct_unsafe() {
        CHECK_CAST(ASTNodeKind::UnnamedStruct);
        return (UnnamedStruct*) this;
    }

    /**
     * return if this is a typealias statement
     */
    inline TypealiasStatement *as_typealias_unsafe() {
        CHECK_CAST(ASTNodeKind::TypealiasStmt);
        return (TypealiasStatement*) this;
    }

    /**
     * return if this is a captured variable
     */
    inline CapturedVariable *as_captured_var_unsafe() {
        CHECK_CAST(ASTNodeKind::CapturedVariable);
        return (CapturedVariable*) this;
    }

    /**
     * return if this is a return statement
     */
    inline ReturnStatement *as_return_unsafe() {
        CHECK_CAST(ASTNodeKind::ReturnStmt);
        return (ReturnStatement*) this;
    }

    /**
     * as interface definition
     */
    inline InterfaceDefinition *as_interface_def_unsafe() {
        CHECK_CAST(ASTNodeKind::InterfaceDecl);
        return (InterfaceDefinition*) this;
    }

    /**
     * as generic func decl
     */
    inline GenericFuncDecl* as_gen_func_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericFuncDecl);
        return (GenericFuncDecl*) this;
    }

    /**
     * as namespace
     */
    inline Namespace* as_namespace_unsafe() {
        CHECK_CAST(ASTNodeKind::NamespaceDecl);
        return (Namespace*) this;
    }

    /**
     * as init block
     */
    inline InitBlock* as_init_block_unsafe() {
        CHECK_CAST(ASTNodeKind::InitBlock);
        return (InitBlock*) this;
    }

    /**
     * get as unsafe block
     */
    inline UnsafeBlock* as_unsafe_block_unsafe() {
        CHECK_CAST(ASTNodeKind::UnsafeBlock);
        return (UnsafeBlock*) this;
    }

    /**
     * return if this is a struct definition
     */
    inline StructDefinition *as_struct_def_unsafe() {
        CHECK_CAST(ASTNodeKind::StructDecl);
        return (StructDefinition*) this;
    }

    /**
     * return if this is a generic struct decl
     */
    inline GenericStructDecl* as_gen_struct_def_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericStructDecl);
        return (GenericStructDecl*) this;
    }

    /**
     * return if this is a generic union decl
     */
    inline GenericUnionDecl* as_gen_union_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericUnionDecl);
        return (GenericUnionDecl*) this;
    }

    /**
     * return if this is a generic interface decl
     */
    inline GenericInterfaceDecl* as_gen_interface_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericInterfaceDecl);
        return (GenericInterfaceDecl*) this;
    }

    /**
     * return if this is a generic variant decl
     */
    inline GenericVariantDecl* as_gen_variant_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericVariantDecl);
        return (GenericVariantDecl*) this;
    }

    /**
     * return if this is a generic variant decl
     */
    inline GenericTypeDecl* as_gen_type_decl_unsafe() {
        CHECK_CAST(ASTNodeKind::GenericTypeDecl);
        return (GenericTypeDecl*) this;
    }

    /**
     * return if this is an import statement
     */
    inline ImportStatement *as_import_stmt_unsafe() {
        CHECK_CAST(ASTNodeKind::ImportStmt);
        return (ImportStatement*) this;
    }

    /**
     * return if this is a implementation def
     */
    inline ImplDefinition* as_impl_def_unsafe() {
        CHECK_CAST(ASTNodeKind::ImplDecl);
        return (ImplDefinition*) this;
    }

    /**
     * return if this is a struct definition
     */
    inline UnionDef *as_union_def_unsafe() {
        CHECK_CAST(ASTNodeKind::UnionDecl);
        return (UnionDef*) this;
    }

    /**
     * return if this is a var init statement
     */
    inline VarInitStatement *as_var_init_unsafe() {
        CHECK_CAST(ASTNodeKind::VarInitStmt);
        return (VarInitStatement*) this;
    }

    /**
     * return if this is a variant member
     */
    inline VariantMember* as_variant_member_unsafe() {
        CHECK_CAST(ASTNodeKind::VariantMember);
        return (VariantMember*) this;
    }

    /**
     * get unsafe pointer to variant member param
     */
    inline VariantMemberParam* as_variant_member_param_unsafe() {
        CHECK_CAST(ASTNodeKind::VariantMemberParam);
        return (VariantMemberParam*) this;
    }

    /**
     * return if this is a variant definition
     */
    inline VariantDefinition* as_variant_def_unsafe() {
        CHECK_CAST(ASTNodeKind::VariantDecl);
        return (VariantDefinition*) this;
    }

    /**
     * return if this is a variant case variable
     */
    inline VariantCaseVariable* as_variant_case_var_unsafe() {
        CHECK_CAST(ASTNodeKind::VariantCaseVariable);
        return (VariantCaseVariable*) this;
    }

    /**
     * return assignment statement if it is one
     */
    inline AssignStatement* as_assignment_unsafe() {
        CHECK_CAST(ASTNodeKind::AssignmentStmt);
        return (AssignStatement*) this;
    }

};