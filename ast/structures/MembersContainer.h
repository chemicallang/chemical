// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include <optional>
#include <map>
#include "StructMember.h"
#include "ast/base/AnnotableNode.h"
#include "FunctionDeclaration.h"
#include "VariablesContainer.h"
#include "MultiFunctionNode.h"

class MembersContainer : public AnnotableNode, public VariablesContainer {
private:

    std::vector<FunctionDeclaration*> functions_container;
    std::unordered_map<chem::string_view, FunctionDeclaration*> indexes;
    std::vector<std::unique_ptr<MultiFunctionNode>> multi_nodes;

    void set_active_iteration_no_subs(int16_t iteration);

public:

    std::vector<GenericTypeParameter*> generic_params;
    /**
     * subscribers are reported with usages of this generic type
     */
    std::vector<GenericType*> subscribers;
    /**
     * generic parameters pretend to be different types on different iterations, iterations are number of usages
     * that we determined during symbol resolution
     */
    int16_t active_iteration = -1;
    /**
     * the iterations for which functions have been declared
     * this is an index, so next starts at this index
     */
    int16_t iterations_declared = 0;
    /**
     * iterations for which function bodies have been generated
     * this is an index, so next starts at this index
     */
    int16_t iterations_body_done = 0;

#ifdef COMPILER_BUILD
    /**
     * here the generic llvm data, which corresponds to generic iterations, for example
     * if user types a generic struct, which contains a generic function
     * the complete specializations of generics will be stored at different indexes
     * where 0 would be the first complete specialization
     * here the vector contains another vector, the contained corresponds to generic iterations of the
     * functions, so that generic functions can be stored inside generic structs
     */
    std::unordered_map<FunctionDeclaration*, std::vector<std::vector<llvm::Function*>>> generic_llvm_data;
#endif

    const std::vector<FunctionDeclaration*>& functions() {
        return functions_container;
    }

    VariablesContainer* as_variables_container() {
        return this;
    }

    void link_signature(SymbolResolver &linker);

    void declare_and_link_no_scope(SymbolResolver &linker);

    void redeclare_inherited_members(SymbolResolver &linker);

    void redeclare_variables_and_functions(SymbolResolver &linker);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr);

    /**
     * this would register the definition to all interfaces inherited
     * directly inherited or indirectly, this definition would be registered
     */
    void register_use_to_inherited_interfaces(StructDefinition* definition);

    FunctionDeclaration *member(const chem::string_view &name);

    ASTNode *child(const chem::string_view &name);

    FunctionDeclaration* inherited_function(const chem::string_view& name);

    FunctionDeclaration *direct_child_function(const chem::string_view& name);

    inline bool is_generic() {
        return !generic_params.empty();
    }

    /**
     * get child variable index, including the inherited types
     */
    int child_index(const chem::string_view &var_name) final {
        return VariablesContainer::variable_index(var_name);
    }

    /**
     * how many actual functions are generated from this generic function
     * non-generic functions return 1
     */
    int16_t total_generic_iterations();

    /**
     * register generic args, but with already existing generic iteration (iteration is returned)
     */
    int16_t register_with_existing(ASTDiagnoser& diagnoser, std::vector<BaseType*>& types);

    /**
     * register the generic args, so code is generated for these types, a generic iteration is returned
     */
    int16_t register_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& types);

    /**
     * register value for the struct
     */
    int16_t register_value(SymbolResolver& resolver, StructValue* structValue);

    /**
     * get the active generic iteration
     */
    int16_t get_active_iteration() final {
        return active_iteration;
    }

    /**
     * set's the active iteration for a generic function
     * this helps generics types pretend to be certain type
     */
    void set_active_iteration(int16_t iteration) final;

    /**
     * set's the generic active iteration safely
     */
    inline void set_active_iteration_safely(int16_t itr) {
        if(itr < -1) return;
        set_active_iteration(itr);
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
    FunctionDeclaration* constructor_func(ASTAllocator& allocator, std::vector<Value*>& forArgs);

    /**
     * will provide a implicit constructor function if there's one for the given value
     * the implicit constructor is used for type conversion
     */
    FunctionDeclaration* implicit_constructor_func(ASTAllocator& allocator, Value* type);

    /**
     * if there's a post move function (clear function), null is returned
     * move function is returned, if there's no move function
     * we return a implicit copy function unless there's none
     */
    FunctionDeclaration* pre_move_func();

    /**
     * checks if any member has a default constructor
     */
    bool any_member_has_def_constructor();

    /**
     * checks if this struct type requires a destructor
     * or has one
     */
    bool any_member_has_destructor();

    /**
     * check if move fn is required
     */
    bool any_member_has_pre_move_func();

    /**
     * check if move fn is required
     */
    bool any_member_has_move_func();

    /**
     * checks if this struct type requires a move function
     * or has one
     */
    bool any_member_has_clear_func();

    /**
     * checks if this struct type requires a copy function
     */
    bool any_member_has_copy_func();

    /**
     * this means struct must be moved by calling move constructor and
     * the default mem copy doesn't suffice
     */
    inline bool requires_moving() {
        return destructor_func() != nullptr || clear_func() != nullptr || pre_move_func() != nullptr;
    }

    /**
     * get the byte size
     */
    virtual uint64_t byte_size(bool is64Bit) = 0;

    /**
     * get the bytesize at the folloing iteration
     */
    uint64_t byte_size(bool is64Bit, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto size = byte_size(is64Bit);
        set_active_iteration(prev);
        return size;
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
    FunctionDeclaration* clear_func();

    /**
     * will provide the move function if there's one
     */
    FunctionDeclaration* move_func();

    /**
     * will provide the move function if there's one
     */
    FunctionDeclaration* copy_func();

    /**
     * insert the given function into this members container
     */
    void insert_func(FunctionDeclaration* decl);

    /**
     * create a destructor function and put it into functions
     */
    FunctionDeclaration* create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name);

    /**
     * create a destructor function and put it into functions
     */
    FunctionDeclaration* create_destructor(ASTAllocator& allocator);

    /**
     * create the move function and put it into functions
     */
    FunctionDeclaration* create_clear_fn(ASTAllocator& allocator);

    /**
     * create the copy function and put it into functions
     */
    FunctionDeclaration* create_copy_fn(ASTAllocator& allocator);

    /**
     * create the copy function and put it into functions
     */
    FunctionDeclaration* create_move_fn(ASTAllocator& allocator);

    /**
     * creates a default constructor, report errors in given diagnoser, this is a helper function
     * the container name here is the name of the struct and not the function
     */
    FunctionDeclaration* create_def_constructor_checking(ASTAllocator& allocator, ASTDiagnoser& diagnoser, const chem::string_view& container_name);

    /**
     * create default destructor, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * create default move function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * create default copy function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * create default move function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * insert a function that can have same name for multiple declarations
     * @return true, if could insert the function, false if there's a conflict
     */
    bool insert_multi_func(FunctionDeclaration* decl);

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
     * this generic type is registered as a subscriber of this generic node
     */
    void subscribe(GenericType *subscriber) final {
        subscribers.emplace_back(subscriber);
    }

    /**
     * check if the other node extends given node
     * for example this container represents a struct X : public Y
     */
    bool extends_node(ASTNode* other);

    /**
     * get the overriding struct / interface and the function being overridden
     */
    std::pair<ASTNode*, FunctionDeclaration*> get_overriding_info(FunctionDeclaration* function);

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
     * for the given struct iteration, we acquire all the function iterations and put them
     * in the llvm_struct types, this basically set's the given iteration so that when llvm_type is called
     * or llvm_value, it will consider the struct iteration
     */
    void acquire_function_iterations(int16_t iteration);

    /**
     * will call code_gen_declare on generic arguments that are structs
     * This ensures that struct's function being called inside generic structs
     * are already declared before we call them
     */
    void early_declare_structural_generic_args(Codegen& gen);

    /**
     * inside a generic struct
     * by giving struct's generic iteration, you can get llvm's function data for the given iteration of the function
     * inside a generic struct
     * where struct iteration means struct is generic and it's complete specialization for that itr
     * where func iteration menas func is generic and it's complete specialization for that itr
     * if function is not generic, just use 0 as func_itr, here if struct is generic, function will not be generic, unless
     * function has other generic parameters that are not present in struct
     */
    llvm::Function*& llvm_generic_func_data(FunctionDeclaration* decl, int16_t struct_itr, int16_t func_itr);

    /**
     * this uses active iteration of both the current members container and given function declaration
     * to get the function's data, works for both even if function is generic or this members container is generic
     * it'll always get the correct func callee and func type
     */
    llvm::Function* llvm_func_data(FunctionDeclaration* decl);

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
     * llvm type for the given generic iteration
     */
    llvm::Type *llvm_type(Codegen &gen, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto type = llvm_type(gen);
        set_active_iteration(prev);
        return type;
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