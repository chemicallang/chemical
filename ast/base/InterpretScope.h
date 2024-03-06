// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>

class Value;

class GlobalInterpretScope;

class InterpretScope {
public:

    explicit InterpretScope(InterpretScope* parent, GlobalInterpretScope* global);

    // delete copy constructor
    InterpretScope(InterpretScope&& copy) = delete;

    /**
     * useful for debugging
     */
    void printAllValues();

    /**
     * The errors are stored in global scope only
     * @param err
     */
    inline void error(const std::string& err);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    ~InterpretScope();

    /**
     * This contains a map between identifiers and its values
     * When a variable is created, the variable sets the identifier in unordered-map
     */
    std::unordered_map<std::string, Value*> values;

    /**
     * a pointer to the parent scope, If this is a global scope, it will be a nullptr
     */
    InterpretScope* parent;

    /**
     * a pointer to global scope, If this is a global scope, it will be a pointer to itself
     */
    GlobalInterpretScope* global;

};