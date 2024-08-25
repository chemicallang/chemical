// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "utils/fwd/functional.h"
#include <vector>
#include <memory>

class Namespace;

class SymbolResolver;

class GlobalInterpretScope : public InterpretScope {
public:

    /**
     * The constructor
     */
    GlobalInterpretScope();

    /**
     * deleted copy constructor
     * @param copy
     */
    GlobalInterpretScope(const GlobalInterpretScope &copy) = delete;

    /**
     * use default move constructor
     */
    GlobalInterpretScope(GlobalInterpretScope&& global) = default;

    /**
     * this method is called by prepare_compiler_namespace automatically
     * this creates the compiler namespace in the global_nodes map
     */
    std::unique_ptr<Namespace>& create_compiler_namespace();

    /**
     * will prepare compiler namespace in the symbol resolver
     * YOU DO NOT NEED TO CALL create_compiler_namespace before this
     * when you call this compiler namespace will declare itself in this symbol resolver
     * if you'd like to change symbol resolver after this call rebind_compiler_namespace
     */
    void prepare_compiler_namespace(SymbolResolver& resolver);

    /**
     * this method can be called after calling prepare_compiler_namespace / create_compiler_namespace
     * it's used to bind a new symbol resolver
     * once on a new symbol resolver to bind with a new symbol resolver
     */
    void rebind_compiler_namespace(SymbolResolver& resolver);

    /**
     * cleans the scope
     * This doesn't clean global functions and values because they are not part of
     * interpretation state !
     */
    void clean() override;

    /**
     * overrides the destructor of InterpretScope
     * this is done because dereferencing "this" in base class for an object of derived class
     * causes segfaults, which could be because of object slicing
     */
    ~GlobalInterpretScope() override;

    /**
     * Given error will be stored in the errors vector
     * @param err
     */
    void add_error(const std::string &err);

    /**
     * global functions that are evaluated during interpretation
     */
    std::unordered_map<std::string, std::unique_ptr<ASTNode>> global_nodes;

    /**
     * global values that are used by global fns
     */
    std::unordered_map<std::string, std::unique_ptr<Value>> global_vals;

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

};