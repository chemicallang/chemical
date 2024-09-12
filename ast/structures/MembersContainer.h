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

    std::vector<std::unique_ptr<FunctionDeclaration>> functions_container;
    std::unordered_map<std::string, FunctionDeclaration*> indexes;
    std::vector<std::unique_ptr<MultiFunctionNode>> multi_nodes;

public:

    std::vector<std::unique_ptr<GenericTypeParameter>> generic_params;
    /**
     * subscribers are reported with usages of this generic type
     */
    std::vector<GenericType*> subscribers;
    /**
     * generic parameters pretend to be different types on different iterations, iterations are number of usages
     * that we determined during symbol resolution, by default zero means no active
     */
    int16_t active_iteration = 0;
    /**
     * the iterations for which code has been generated
     * this is an index, so it means you must generate the code beginning at this iteration
     * because before that, all iterations have their code generated
     */
    int16_t iterations_done = 0;

#ifdef COMPILER_BUILD
    /**
     * here the generic llvm data, which corresponds to generic iterations, for example
     * if user types a generic struct, which contains a generic function
     * the complete specializations of generics will be stored at different indexes
     * where 0 would be the first complete specialization
     * here the vector contains another vector, the contained corresponds to generic iterations of the
     * functions, so that generic functions can be stored inside generic structs
     */
    std::unordered_map<FunctionDeclaration*, std::vector<std::vector<std::pair<llvm::Value*, llvm::FunctionType*>>>> generic_llvm_data;
#endif

    const std::vector<std::unique_ptr<FunctionDeclaration>>& functions() {
        return functions_container;
    }

    VariablesContainer* as_variables_container() override {
        return this;
    }

    void declare_and_link_no_scope(SymbolResolver &linker);

    void redeclare_inherited_members(SymbolResolver &linker);

    void redeclare_variables_and_functions(SymbolResolver &linker);

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    /**
     * this would register the definition to all interfaces inherited
     * directly inherited or indirectly, this definition would be registered
     */
    void register_use_to_inherited_interfaces(StructDefinition* definition);

    FunctionDeclaration *member(const std::string &name);

    ASTNode *child(const std::string &name) override;

    BaseDefMember *direct_child_member(const std::string& name);

    BaseDefMember *inherited_member(const std::string& name);

    BaseDefMember *child_member(const std::string& name);

    FunctionDeclaration *direct_child_function(const std::string& name);

    /**
     * get child variable index, including the inherited types
     */
    int child_index(const std::string &var_name) override {
        return VariablesContainer::variable_index(var_name);
    }

    /**
     * how many actual functions are generated from this generic function
     * non-generic functions return 1
     */
    int16_t total_generic_iterations();

    /**
     * register the generic args, so code is generated for these types, a generic iteration is returned
     */
    int16_t register_generic_args(SymbolResolver& resolver, std::vector<std::unique_ptr<BaseType>>& types);

    /**
     * register value for the struct
     */
    int16_t register_value(SymbolResolver& resolver, StructValue* structValue);

    /**
     * get the active generic iteration
     */
    int16_t get_active_iteration() override {
        return active_iteration;
    }

    /**
     * set's the active iteration for a generic function
     * this helps generics types pretend to be certain type
     */
    void set_active_iteration(int16_t iteration) override;

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
    FunctionDeclaration* get_first_fn_annotated(AnnotationKind);

    /**
     * get last function with annotation
     */
    FunctionDeclaration* get_last_fn_annotated(AnnotationKind);

    /**
     * will check if any function with constructor annotation exists
     */
    bool has_constructor() {
        return get_first_fn_annotated(AnnotationKind::Constructor) != nullptr;
    }

    /**
     * will provide a constructor function if there's one
     */
    FunctionDeclaration* constructor_func(std::vector<std::unique_ptr<Value>>& forArgs);

    /**
     * will provide a implicit constructor function if there's one for the given value
     * the implicit constructor is used for type conversion
     */
    FunctionDeclaration* implicit_constructor_func(Value* type);

    /**
     * checks if this struct type requires a destructor
     * or has one
     */
    bool requires_destructor();

    /**
     * checks if this struct type requires a move function
     * or has one
     */
    bool requires_clear_fn();

    /**
     * this means struct must be moved by calling move constructor and
     * the default mem copy doesn't suffice
     */
    inline bool requires_moving() {
        return requires_destructor() || requires_clear_fn();
    }

    /**
     * checks if this struct type requires a copy function
     */
    bool requires_copy_fn();

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
     * will provide a destructor function if there's one
     */
    FunctionDeclaration* destructor_func() {
        return get_last_fn_annotated(AnnotationKind::Delete);
    }

    /**
     * will provide the move function if there's one
     */
    FunctionDeclaration* clear_func() {
        return get_last_fn_annotated(AnnotationKind::Clear);
    }

    /**
     * will provide the move function if there's one
     */
    FunctionDeclaration* copy_func() {
        return get_last_fn_annotated(AnnotationKind::Copy);
    }

    /**
     * insert the given function into this members container
     */
    void insert_func(std::unique_ptr<FunctionDeclaration> decl);

    /**
     * create a destructor function and put it into functions
     */
    FunctionDeclaration* create_destructor();

    /**
     * create the move function and put it into functions
     */
    FunctionDeclaration* create_clear_fn();

    /**
     * create the copy function and put it into functions
     */
    FunctionDeclaration* create_copy_fn();

    /**
     * create default destructor, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_destructor(ASTDiagnoser& diagnoser);

    /**
     * create default move function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_clear_fn(ASTDiagnoser& diagnoser);

    /**
     * create default copy function, report errors in given diagnoser, this is a helper function
     */
    FunctionDeclaration* create_def_copy_fn(ASTDiagnoser& diagnoser);

    /**
     * insert a function that can have same name for multiple declarations
     * @return true, if could insert the function, false if there's a conflict
     */
    bool insert_multi_func(std::unique_ptr<FunctionDeclaration> decl);

    /**
     * is there a function with this name
     */
    bool contains_func(const std::string& name);

    /**
     * this generic type is registered as a subscriber of this generic node
     */
    void subscribe(GenericType *subscriber) override {
        subscribers.emplace_back(subscriber);
    }

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
     * inside a generic struct
     * by giving struct's generic iteration, you can get llvm's function data for the given iteration of the function
     * inside a generic struct
     * where struct iteration means struct is generic and it's complete specialization for that itr
     * where func iteration menas func is generic and it's complete specialization for that itr
     * if function is not generic, just use 0 as func_itr, here if struct is generic, function will not be generic, unless
     * function has other generic parameters that are not present in struct
     */
    std::pair<llvm::Value*, llvm::FunctionType*>& llvm_generic_func_data(FunctionDeclaration* decl, int16_t struct_itr, int16_t func_itr);

    /**
     * this uses active iteration of both the current members container and given function declaration
     * to get the function's data, works for both even if function is generic or this members container is generic
     * it'll always get the correct func callee and func type
     */
    std::pair<llvm::Value*, llvm::FunctionType*> llvm_func_data(FunctionDeclaration* decl);

    /**
     * add child index
     */
    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    ) override {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

    /**
     * llvm_type below doesn't work without this declaration
     */
    llvm::Type* llvm_type(Codegen &gen) override {
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