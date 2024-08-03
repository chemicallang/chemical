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
     * generic parameters pretend to be different types on different iterations, iterations are number of usages
     * that we determined during symbol resolution, by default zero means no active
     */
    int16_t active_iteration = 0;

    const std::vector<std::unique_ptr<FunctionDeclaration>>& functions() {
        return functions_container;
    }

    void declare_and_link(SymbolResolver &linker) override;

    FunctionDeclaration *member(const std::string &name);

    ASTNode *child(const std::string &name) override;

    int child_index(const std::string &var_name) override {
        return VariablesContainer::variable_index(var_name);
    }

    /**
     * how many actual functions are generated from this generic function
     * non-generic functions return 1
     */
    int16_t total_generic_iterations();

    /**
     * register value for the struct
     */
    int16_t register_value(StructValue* structValue);

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
     * will provide a constructor function if there's one
     */
    FunctionDeclaration* constructor_func(std::vector<std::unique_ptr<Value>>& forArgs);

    /**
     * will provide a implicit constructor function if there's one for the given value
     * the implicit constructor is used for type conversion
     */
    FunctionDeclaration* implicit_constructor_func(Value* type);

    /**
     * checks if this struct type requires destructor
     */
    bool requires_destructor();

    /**
     * will provide a destructor function if there's one
     */
    FunctionDeclaration* destructor_func();

    /**
     * insert the given function into this members container
     */
    void insert_func(std::unique_ptr<FunctionDeclaration> decl);

    /**
     * insert a function that can have same name for multiple declarations
     * @return true, if could insert the function, false if there's a conflict
     */
    bool insert_multi_func(std::unique_ptr<FunctionDeclaration> decl);

    /**
     * is there a function with this name
     */
    bool contains_func(const std::string& name);



#ifdef COMPILER_BUILD

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    ) override {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

};