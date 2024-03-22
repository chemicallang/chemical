// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include <vector>

class GlobalInterpretScope : public InterpretScope {
public:

    /**
     * The constructor
     */
    GlobalInterpretScope(InterpretScope *parent, Scope *scope, ASTNode *node, std::string path);

    /**
     * deleted copy constructor
     * @param copy
     */
    GlobalInterpretScope(const GlobalInterpretScope &copy) = delete;

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
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

    /**
     * root path is the path of the file, interpretation begun at
     */
    std::string root_path;

};