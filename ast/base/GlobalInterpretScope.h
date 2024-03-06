// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"

class GlobalInterpretScope : public InterpretScope {
public:

    /**
     * The constructor
     */
    GlobalInterpretScope(Scope* scope, ASTNode* node);

    // delete copy constructor
    GlobalInterpretScope(GlobalInterpretScope&& copy) = delete;

    /**
     * Given error will be stored in the errors vector
     * @param err
     */
    void add_error(const std::string &err);

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

};