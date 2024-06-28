// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>

class Value;

class GlobalInterpretScope;

class Scope;

class ASTNode;

using node_map = std::unordered_map<std::string, ASTNode*>;
using node_iterator = node_map::iterator;
using value_map = std::unordered_map<std::string, Value*>;
using value_iterator = value_map::iterator;

class InterpretScope {
public:

    explicit InterpretScope(InterpretScope* parent, GlobalInterpretScope* global);

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& scope) = default;

    /**
     * deleted copy constructor
     * @param copy
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * declares a value with this name in current scope
     */
    void declare(const std::string& name, Value* value);

    /**
     * erases a value by the key name from the value map safely
     */
    void erase_value(const std::string &name);

    /**
     * @return return value with name, or nullptr
     */
    Value* find_value(const std::string& name);

    /**
     * @return iterator for found value, the map that it was found in
     */
    std::pair<value_iterator, value_map&> find_value_iterator(const std::string& name);

    /**
     * print all values
     */
    void print_values();

    /**
     * The errors are stored in global scope only
     * @param err
     */
    void error(const std::string& err);

    /**
     * this can be called to "clean" everything in this scope
     * to make it reusable
     */
    virtual void clean();

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    virtual ~InterpretScope();

    /**
      * This contains a map between identifiers and its values, of the current scope
      */
    std::unordered_map<std::string, Value *> values;

    /**
     * a pointer to the parent scope, If this is a global scope, it will be a nullptr
     */
    InterpretScope* parent;

    /**
     * a pointer to global scope, If this is a global scope, it will be a pointer to itself
     */
    GlobalInterpretScope* global;

};