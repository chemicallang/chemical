// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "std/common.h"
#include "ASTAny.h"
#include "BaseTypeKind.h"
#include "std/hybrid_ptr.h"
#include "ASTNodeKind.h"
#include "AccessSpecifier.h"
#include "ASTAllocator.h"
#include <optional>

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

class BaseFunctionParam;

class AnnotableNode;

class MultiFunctionNode;

class VariablesContainer;

class MembersContainer;

class GenericTypeParameter;

class BaseDefMember;

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode : public ASTAny {
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
     * any kind of 'node' is returned
     */
    ASTAnyKind any_kind() final {
        return ASTAnyKind::Node;
    }

    /**
     * declare something on the scope map
     * that must be retained in nested level scopes
     * for example top level functions can be called within functions
     */
    virtual void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
        // does nothing by default
    }

    /**
     * called in case some functions need to redeclare themselves
     */
    virtual void redeclare_top_level(SymbolResolver &linker) {
        ASTNode* node;
        declare_top_level(linker, node);
    }

    /**
     * declares something on the scope map
     * or find something on the map to link yourself with it
     */
    virtual void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
        // does nothing by default
    }

    /**
     * this function provides a pointer to the parent ASTNode
     * a var init inside for loop, gets a pointer to the for loop
     */
    virtual ASTNode *parent() = 0;

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
     * this function provides a pointer to the parent ASTNode
     * a var init inside for loop, gets a pointer to the for loop
     */
    virtual void set_parent(ASTNode*);

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
    virtual ASTNode *child(const chem::string_view &name) {
        return nullptr;
    }

    /**
     * same as child, only it returns the index of the child
     * so it can be used to create get element pointer instructions using llvm
     */
    virtual int child_index(const chem::string_view &name) {
        return -1;
    }

    /**
     * get the ast node kind from this node
     */
    virtual ASTNodeKind kind() = 0;

    /**
     * this generic type would subscribe to this node, so all usages of this generic node
     * will be registered with this subscriber
     */
    virtual void subscribe(GenericType* subscriber);

    /**
     * get active iteration, if the underlying node doesn't support iteration -1 is returned
     */
    virtual int16_t get_active_iteration();

    /**
     * set's active iteration, this method will fail, if the given
     * node doesn't support generics
     */
    virtual void set_active_iteration(int16_t iteration);

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
    MembersContainer* get_members_container(ASTNodeKind k);

    /**
     * This would return the representation of the node
     */
    std::string representation();

    /**
     * runtime name will be written to the given stream
     * runtime name is constructed by prepending parent's names
     * into the final name
     */
    virtual void runtime_name(std::ostream& stream);

    /**
     * runtime name of this node without any parent names appended to it
     */
    virtual void runtime_name_no_parent(std::ostream& stream);

    /**
     * get runtime_name as a string
     */
    std::string runtime_name_str();

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
     * get the type from the ASTNode
     * this type can represent the type of value, type of parameter etc.
     */
    virtual BaseType* create_value_type(ASTAllocator& allocator);

    /**
     * this returns a hybrid pointer, which decreases the number of allocations, because
     * type of value may be known by the value
     */
    inline BaseType* get_value_type(ASTAllocator& allocator) {
        return create_value_type(allocator);
    }

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
    virtual void llvm_destruct(Codegen& gen, llvm::Value* allocaInst) {
        // no destruction required
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
    virtual void code_gen_destruct(Codegen &gen, Value* returnValue) {
        // does nothing by default
    }

    /**
     * this function allows us to code_gen for imported generics
     * generic nodes emit code once, their usage is detected at symbol resolution, however
     * when generic nodes are imported from other files, they can't emit code, because code_gen
     * is not called on them, however this function is called on imported nodes, that registered
     * themselves in symbol resolver because of another usage for which code wasn't generated before
     * function checks strictly that no code is generated for already present functions
     */
    virtual void code_gen_generic(Codegen &gen);

    /**
     * add child index in llvm indexes vector
     */
    virtual bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name);

    /**
     * loads the value of the given ASTNode
     * this is called by variable identifier, on linked nodes (var init, function parameters)
     */
    virtual llvm::Value *llvm_load(Codegen &gen);

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
    [[nodiscard]]
    virtual BaseTypeKind type_kind() const {
        return BaseTypeKind::Unknown;
    }

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

    static inline constexpr bool isCommentStmt(ASTNodeKind k) {
        return k == ASTNodeKind::CommentStmt;
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
        return k == ASTNodeKind::FunctionDecl || k == ASTNodeKind::ExtensionFunctionDecl;
    }

    static inline constexpr bool isExtensionFunctionDecl(ASTNodeKind k) {
        return k == ASTNodeKind::ExtensionFunctionDecl;
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

    static inline constexpr bool isExtensionFuncReceiver(ASTNodeKind k) {
        return k == ASTNodeKind::ExtensionFuncReceiver;
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
        return k == ASTNodeKind::ExtensionFuncReceiver || k == ASTNodeKind::FunctionParam;
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
     * return if this is a base function paam
     */
    inline BaseFunctionParam* as_base_func_param() {
        return isBaseFuncParam(kind()) ? (BaseFunctionParam*) this : nullptr;
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
     * get as extension function
     */
    inline ExtensionFunction* as_extension_func() {
        return isExtensionFunctionDecl(kind()) ? (ExtensionFunction*) this : nullptr;
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
        return (ValueWrapperNode*) this;
    }

    /**
     * as loop node unsafe
     */
    inline LoopASTNode* as_loop_node_unsafe() {
        return (LoopASTNode*) this;
    }

    /**
     * as break stmt unsafe
     */
    inline BreakStatement* as_break_stmt_unsafe() {
        return (BreakStatement*) this;
    }

    /**
     * get as if statement unsafely
     */
    inline IfStatement* as_if_stmt_unsafe() {
        return (IfStatement*) this;
    }

    /**
     * get as switch stmt unsafely
     */
    inline SwitchStatement* as_switch_stmt_unsafe() {
        return (SwitchStatement*) this;
    }

    /**
      * return this as an annotable node
      */
    inline AnnotableNode* as_annotable_node_unsafe() {
        return (AnnotableNode*) this;
    }

    /**
     * return if this is definition member
     */
    inline BaseDefMember* as_base_def_member_unsafe() {
        return (BaseDefMember*) this;
    }

    /**
     * get a members container
     */
    inline MembersContainer* as_members_container_unsafe() {
        return (MembersContainer*) this;
    }

    /**
     * return if this is a loop ast node
     */
    inline LoopASTNode *as_loop_ast_unsafe() {
        return (LoopASTNode*) this;
    }

    /**
     * return if this is a base function paam
     */
    inline BaseFunctionParam* as_base_func_param_unsafe() {
        return (BaseFunctionParam*) this;
    }

    /**
     * return if this is a scope
     */
    inline Scope *as_scope_unsafe() {
        return (Scope*) this;
    }

    /**
     * return this as a generic type parameter if its one
     */
    inline GenericTypeParameter* as_generic_type_param_unsafe() {
        return (GenericTypeParameter*) this;
    }

    /**
     * return this as a multi function node
     */
    inline MultiFunctionNode* as_multi_func_node_unsafe() {
        return (MultiFunctionNode*) this;
    }

    /**
     * as enum member
     */
    inline EnumDeclaration* as_enum_decl_unsafe() {
        return (EnumDeclaration*) this;
    }

    /**
     * as enum member
     */
    inline EnumMember* as_enum_member_unsafe() {
        return (EnumMember*) this;
    }

    /**
     * get as extension function
     */
    inline ExtensionFunction* as_extension_func_unsafe() {
        return (ExtensionFunction*) this;
    }

    /**
     * return if this is a parameter
     */
    inline FunctionParam *as_func_param_unsafe() {
        return (FunctionParam*) this;
    }

    /**
     * return if this is a function decl
     * @return
     */
    inline FunctionDeclaration *as_function_unsafe() {
        return (FunctionDeclaration*) this;
    }

    /**
     * return if this is a struct member
     */
    inline StructMember *as_struct_member_unsafe() {
        return (StructMember*) this;
    }

    /**
     * return if this is an unnamed union
     */
    inline UnnamedUnion *as_unnamed_union_unsafe() {
        return (UnnamedUnion*) this;
    }

    /**
     * return if this is an unnamed struct
     */
    inline UnnamedStruct *as_unnamed_struct_unsafe() {
        return (UnnamedStruct*) this;
    }

    /**
     * return if this is a typealias statement
     */
    inline TypealiasStatement *as_typealias_unsafe() {
        return (TypealiasStatement*) this;
    }

    /**
     * return if this is a captured variable
     */
    inline CapturedVariable *as_captured_var_unsafe() {
        return (CapturedVariable*) this;
    }

    /**
     * return if this is a return statement
     */
    inline ReturnStatement *as_return_unsafe() {
        return (ReturnStatement*) this;
    }

    /**
     * as interface definition
     */
    inline InterfaceDefinition *as_interface_def_unsafe() {
        return (InterfaceDefinition*) this;
    }

    /**
     * as namespace
     */
    inline Namespace* as_namespace_unsafe() {
        return (Namespace*) this;
    }

    /**
     * as init block
     */
    inline InitBlock* as_init_block_unsafe() {
        return (InitBlock*) this;
    }

    /**
     * return if this is a struct definition
     */
    inline StructDefinition *as_struct_def_unsafe() {
        return (StructDefinition*) this;
    }

    /**
     * return if this is an import statement
     */
    inline ImportStatement *as_import_stmt_unsafe() {
        return (ImportStatement*) this;
    }

    /**
     * return if this is a implementation def
     */
    inline ImplDefinition* as_impl_def_unsafe() {
        return (ImplDefinition*) this;
    }

    /**
     * return if this is a struct definition
     */
    inline UnionDef *as_union_def_unsafe() {
        return (UnionDef*) this;
    }

    /**
     * return if this is a var init statement
     */
    inline VarInitStatement *as_var_init_unsafe() {
        return (VarInitStatement*) this;
    }

    /**
     * return if this is a variant member
     */
    inline VariantMember* as_variant_member_unsafe() {
        return (VariantMember*) this;
    }

    /**
     * get unsafe pointer to variant member param
     */
    inline VariantMemberParam* as_variant_member_param_unsafe() {
        return (VariantMemberParam*) this;
    }

    /**
     * return if this is a variant definition
     */
    inline VariantDefinition* as_variant_def_unsafe() {
        return (VariantDefinition*) this;
    }

    /**
     * return if this is a variant case variable
     */
    inline VariantCaseVariable* as_variant_case_var_unsafe() {
        return (VariantCaseVariable*) this;
    }

    /**
     * return assignment statement if it is one
     */
    inline AssignStatement* as_assignment_unsafe() {
        return (AssignStatement*) this;
    }

};

static_assert(sizeof(ASTNode) <= 8, "ASTNode must always be equal or less than 8 bytes");