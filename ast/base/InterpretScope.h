// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>
#include "ast/base/ASTAllocator.h"

class Value;

class GlobalInterpretScope;

class Scope;

class ASTNode;

using node_map = std::unordered_map<std::string, ASTNode*>;
using node_iterator = node_map::iterator;
using value_map = std::unordered_map<std::string, Value*>;
using value_iterator = value_map::iterator;

class FunctionType;

class InterpretScope {
public:

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

    /**
     * this is a very lightweight allocator, that allocates every value on heap
     * that's it
     */
    ASTAllocator& allocator;

    /**
     * the pointers allocated by this scope, we call destruct
     * on these pointers when the scope dies
     */
    std::vector<ASTAny*> allocated;

    /**
     * constructor
     */
    explicit InterpretScope(
        InterpretScope* parent,
        ASTAllocator& allocator,
        GlobalInterpretScope* global
    );

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& scope) noexcept = default;

    /**
     * deleted copy constructor
     * @param copy
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * a helper function to allocate objects so they are destroyed when the scope dies
     * instead of when the allocator dies
     */
    template<typename T>
    inline T* allocate() {
        const auto ptr = allocator.allocate_released<T>();
        allocated.emplace_back(ptr);
        return ptr;
    }

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
    std::pair<value_iterator, InterpretScope&> find_value_iterator(const std::string& name);

    /**
     * print all values
     */
    void print_values();

    /**
     * The errors are stored in global scope only
     */
    void error(std::string& err, ASTAny* any);

    /**
     * an interpret scope error
     */
    void error(std::string_view err, ASTAny* any);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    virtual ~InterpretScope();

};