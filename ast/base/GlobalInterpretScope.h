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
    GlobalInterpretScope(InterpretScope* parent, Scope* scope, ASTNode* node, std::string  path);

    // delete copy constructor
    GlobalInterpretScope(GlobalInterpretScope&& copy) = delete;

    /**
     * Given error will be stored in the errors vector
     * @param err
     */
    void add_error(const std::string &err);

    /**
     * When a ASTNode declares itself, for example a struct, interface / implementation
     * it declares itself on this unordered map, when the scope ends, it erases itself from this map
     */
    std::unordered_map<std::string, ASTNode*> nodes;

    /**
      * This contains a map between identifiers and its values
      * When a child scope is interpreting, it puts values on this map, when it ends, only the values of that scope are erased from this map
      * When a variable is created, the variable sets the identifier in this unordered map with the corresponding value
      */
    std::unordered_map<std::string, Value*> values;

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

    /**
     * root path is the path of the file, interpretation begun at
     */
     std::string root_path;

};