// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include <optional>
#include <map>
#include "StructMember.h"
#include "ast/base/AnnotableNode.h"
#include "FunctionDeclaration.h"
#include "GenericFuncDecl.h"
#include "VariablesContainer.h"
#include "MultiFunctionNode.h"
#include <span>
#include "./MembersIterators.h"


class MembersContainer;

class GenericMembersDecl;

struct FunctionOverridingInfo {

    /**
     * the inherited type which corresponds to the base container
     */
    InheritedType* type;

    /**
     * The container (interface or struct) that contains the actual function which is being overridden by us
     */
    MembersContainer* base_container;

    /**
     * the base function present in base container, which is being overridden by us
     */
    FunctionDeclaration* base_func;

};

class MembersContainer : public ASTNode, public VariablesContainer {
private:

    /**
     * the functions container can contain generic function declarations
     * so we are going to use ASTNode*
     */
    std::vector<ASTNode*> functions_container;

public:

    /**
     * this is set by generic declarations that invoke this container as implementation
     */
    GenericMembersDecl* generic_parent = nullptr;

    /**
     * this is the generic instantiation that is instantiated by the generic parent
     */
    int generic_instantiation = -1;

    /**
     * a code gen helper flag, to check if container has been declared
     */
    bool has_declared = false;

    /**
     * a code gen helper flag, to check if container has been implemented
     */
    bool has_implemented = false;

private:

    /**
     * set during symbol resolution
     * all variables + inherited structs in this container can be default initialized
     * using default constructors or default values
     */
    std::optional<bool> default_initialized = std::nullopt;

public:

    /**
     * default constructor
     */
    inline explicit MembersContainer(
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation loc
    ) noexcept : ASTNode(k, parent, loc) {
        // does nothing
    }

    /**
     * this will allow you to iterate over all the non generic functions and all the
     * instantiated functions from generic functions
     */
    InstFuncRange instantiated_functions() {
        return InstFuncRange(functions_container);
    }

    /**
     * this will allow you to iterate over all the non generic functions, for the generic
     * functions only their master function (which is the blueprint is iterated over)
     */
    MasterFuncRange master_functions() {
        return MasterFuncRange(functions_container);
    }

    /**
     * this will allow you to iterate over only the nodes that are function decl or
     * generic function decl
     */
    FuncNodeRange func_nodes_range() {
        return FuncNodeRange(functions_container);
    }

    /**
     * non generic functions ranage
     */
    NonGenFuncRange non_gen_range() {
        return NonGenFuncRange(functions_container);
    }

    /**
     * this gives you the vector of the functions, which could include generic or non generic
     * functions therefore it's a ASTNode* from which you can get the kind and reinterpret_cast
     */
    const std::vector<ASTNode*>& functions() {
        return (std::vector<ASTNode*>&) functions_container;
    }

    /**
     * these are actual nodes that we found after evaluting compile time ifs and all that
     */
    std::vector<ASTNode*>& evaluated_nodes() {
        return functions_container;
    }

    VariablesContainer* as_variables_container() {
        return this;
    }

    bool getAllMembersDefaultInitialized();

    inline void setAllMembersDefaultInitialized(bool value) {
        default_initialized = value;
    }

    /**
     * this method will automatically take variables from parsed nodes
     */
    void take_members_from_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& nodes);

    inline void take_members_from_parsed_nodes(SymbolResolver& linker) {
        if(!nodes.empty()) {
            take_members_from_parsed_nodes(linker, nodes);
        }
    }

    void declare_inherited_members(SymbolResolver& linker);

    void redeclare_inherited_members(SymbolResolver &linker);

    void redeclare_variables_and_functions(SymbolResolver &linker);

    /**
     * this would register the definition to all interfaces inherited
     * directly inherited or indirectly, this definition would be registered
     */
    void register_use_to_inherited_interfaces(StructDefinition* definition);

    FunctionDeclaration* inherited_function(const chem::string_view& name);

    FunctionDeclaration *direct_child_function(const chem::string_view& name);

    inline FunctionDeclaration *member(const chem::string_view &name) {
        return direct_child_function(name);
    }

    /**
     * get first function with annotation
     */
    FunctionDeclaration* get_first_constructor();

    /**
     * will check if any function with constructor annotation exists
     */
    inline bool has_constructor() {
        return get_first_constructor() != nullptr;
    }

    /**
     * default constructor function is the first function without any explicit arguments
     */
    FunctionDeclaration* default_constructor_func();

    /**
     * will provide a constructor function if there's one
     */
    FunctionDeclaration* constructor_func(std::vector<Value*>& forArgs);

    /**
     * will provide a implicit constructor function if there's one for the given value
     * the implicit constructor is used for type conversion
     */
    FunctionDeclaration* implicit_constructor_func(Value* type);

    /**
     * checks if any member has a default constructor
     */
    bool all_members_has_def_constructor();

    /**
     * checks if this struct type requires a destructor
     * or has one
     */
    bool any_member_has_destructor();

    /**
     * checks if this struct type requires a copy function
     */
    bool any_member_has_copy_func();

    /**
     * this means struct must be moved by calling move constructor and
     * the default mem copy doesn't suffice
     */
    inline bool requires_moving() {
        return destructor_func() != nullptr;
    }

    /**
     * get the byte size
     */
    virtual uint64_t byte_size(bool is64Bit) = 0;

    /**
     * shallow copy this container
     */
    void shallow_copy_functions_into(MembersContainer& other, ASTAllocator& allocator) {
        other.functions_container.reserve(functions_container.size());
        for(auto& func : functions_container) {
            switch(func->kind()) {
                case ASTNodeKind::FunctionDecl:{
                    const auto func_copy = func->as_function_unsafe()->shallow_copy(allocator);
                    func_copy->set_parent(&other);
                    other.insert_func(func_copy);
                    break;
                }
                case ASTNodeKind::GenericFuncDecl: {
                    const auto func_copy = func->as_gen_func_decl_unsafe()->shallow_copy(allocator);
                    func_copy->set_parent(&other);
                    other.insert_func(func_copy);
                    break;
                }
                default:
                    break;
            }
        }
    }

    /**
     * shallow copies this container into the given container (including functions, variables)
     */
    inline void shallow_copy_into(MembersContainer& other, ASTAllocator& allocator) {
        VariablesContainer::shallow_copy_into(other, allocator);
        shallow_copy_functions_into(other, allocator);
    }

    /**
     * required size for initializing this struct using values
     */
    unsigned int init_values_req_size();

    /**
     * will provide a destructor function if there's one
     */
    FunctionDeclaration* destructor_func();

    /**
     * will provide the move function if there's one
     */
    FunctionDeclaration* copy_func();

    /**
     * insert the given function into this members container
     */
    void insert_func(FunctionDeclaration* decl);

    /**
     * insert the given generic function into this members container
     */
    void insert_func(GenericFuncDecl* decl);

    /**
     * will insert all the given functions
     */
    void insert_functions(const std::initializer_list<FunctionDeclaration*>& decls);

    /**
     * create a destructor function and put it into functions
     */
    FunctionDeclaration* create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name, ASTNode* returnNode);

    /**
     * create a destructor function and put it into functions
     */
    inline FunctionDeclaration* create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name) {
        return create_def_constructor(allocator, parent_name, this);
    }

    /**
     * create a destructor function and put it into functions
     */
    inline FunctionDeclaration* create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name, GenericStructDecl* decl) {
        return create_def_constructor(allocator, parent_name, (ASTNode*) decl);
    }

    /**
     * create a destructor function and put it into functions
     */
    FunctionDeclaration* create_destructor(ASTAllocator& allocator, ASTNode* returnNode);

    /**
     * create the copy function and put it into functions
     */
    FunctionDeclaration* create_copy_fn(ASTAllocator& allocator, ASTNode* returnNode);

    /**
     * creates a default constructor, report errors in given diagnoser, this is a helper function
     * the container name here is the name of the struct and not the function
     */
    FunctionDeclaration* create_def_constructor_checking(ASTAllocator& allocator, ASTDiagnoser& diagnoser, const chem::string_view& container_name, ASTNode* returnNode);

    /**
     * create default destructor, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode);

    /**
     * create default copy function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode);

    /**
     * insert a function that can have same name for multiple declarations
     * @return true, if could insert the function, false if there's a conflict
     */
    bool insert_multi_func(ASTAllocator& astAllocator, FunctionDeclaration* decl);

    /**
     * is there a function with this name
     */
    bool contains_func(const chem::string_view& name);

    /**
     * this creates a linked type to this members container, so that
     * generic parameters are accounted for
     */
    BaseType* create_linked_type(const chem::string_view& name, ASTAllocator& allocator);

    /**
     * check if the other node extends given node
     * for example this container represents a struct X : public Y
     */
    bool extends_node(ASTNode* other);

    /**
     * get the overriding info for the given function (to know which function is the given function overriding and in which interface / struct)
     */
    FunctionOverridingInfo get_func_overriding_info(FunctionDeclaration* function);

    /**
     * get the overriding struct / interface and the function being overridden
     * @deprecated
     */
    [[deprecated]]
    std::pair<ASTNode*, FunctionDeclaration*> get_overriding_info(FunctionDeclaration* function) {
        const auto info = get_func_overriding_info(function);
        return { info.base_container, info.base_func };
    }

    /**
     * get a function with signature equal to given func type, present in direct or inherited functions
     * it also checks for the function name
     */
    std::pair<ASTNode*, FunctionDeclaration*> get_func_with_signature(FunctionDeclaration* function);

    /**
     * get the function being overridden of this struct, the interface whose function
     */
    FunctionDeclaration* get_overriding(FunctionDeclaration* function);

    /**
     * get the interface overriding info, this means that the function being overridden is present in an interface
     */
    std::pair<InterfaceDefinition*, FunctionDeclaration*> get_interface_overriding_info(FunctionDeclaration* function);

    /**
     * get overriding interface for the following function, means function being overridden is present in an interface
     */
    InterfaceDefinition* get_overriding_interface(FunctionDeclaration* function);


#ifdef COMPILER_BUILD

    /**
     * only call this when you need to declare the functions inside the members container
     * should be called in code_gen_external_declare, which is called on nodes to declare themselves
     * in another module
     */
    void external_declare(Codegen& gen);

    /**
     * add child index
     */
    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    ) {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

    /**
     * llvm_type below doesn't work without this declaration
     */
    llvm::Type* llvm_type(Codegen &gen) {
        return ASTAny::llvm_type(gen);
    }

    /**
     * build the type for llvm vtable, from inherited interfaces
     */
    void llvm_build_inherited_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types);

    /**
     * build llvm vtable, from inherited interfaces
     */
    void llvm_build_inherited_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers);

#endif

};