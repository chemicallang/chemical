// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "utils/fwd/functional.h"
#include <vector>
#include <memory>

typedef Value* (*EvaluatorFn)(Value*, Value*);

using expression_evaluators = std::unordered_map<int, EvaluatorFn>;

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
     * will prepare compiler functions in the symbol resolver
     * namely the compiler namespace, which allows to interact with compiler api
     * with less complexity than CBI
     */
    void prepare_compiler_functions(SymbolResolver& resolver);

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
     * expression evaluators evaluate expressions
     * indexed functions that evaluate two values into another value are put on this map
     * usually index is determined based on the type of values inputted
     */
    expression_evaluators expr_evaluators;

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

};