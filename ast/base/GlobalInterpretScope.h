// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "utils/fwd/functional.h"
#include <vector>
#include <memory>

class BackendContext;

class LabBuildCompiler;

class Namespace;

class SymbolResolver;

class GlobalInterpretScope : public InterpretScope {
public:

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

    /**
     * a pointer to build compiler is stored, so compile time
     * function calls can talk to the compiler (get definitions)
     */
    LabBuildCompiler* build_compiler;

    /**
     * a pointer to backend context is stored, so compile time
     * function calls can generate code based on the backend
     */
    BackendContext* backend_context;

    /**
     * The constructor
     */
    explicit GlobalInterpretScope(BackendContext* backendContext, LabBuildCompiler* buildCompiler);

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
     * this will prepare std and compiler namespace and put it in this symbol resolver
     */
    void prepare_top_level_namespaces(SymbolResolver& resolver);

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

};