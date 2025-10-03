// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>
#include "ast/base/ASTAllocator.h"
#include "std/chem_string_view.h"
#include "ast/utils/Operation.h"
#include "ast/base/ValueKind.h"

class Value;

class GlobalInterpretScope;

class Scope;

class ASTNode;

class LocationManager;

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
     * constructor
     */
    explicit InterpretScope(
        InterpretScope* parent,
        ASTAllocator& allocator,
        GlobalInterpretScope* global
    ) : parent(parent), allocator(allocator), global(global) {

    }

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& scope) noexcept = default;

    /**
     * deleted copy constructor
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * a helper function to allocate objects so they are destroyed when the scope dies
     * instead of when the allocator dies
     */
    template<typename T>
    FORCE_INLINE T* allocate() {
        return allocator.allocate<T>();
    }

    /**
     * declares a value with this name in current scope
     */
    void declare(std::string& name, Value* value);

    /**
     * declares a value with this name in current scope
     */
    void declare(const chem::string_view& name, Value* value);

    /**
     * erases a value by the key name from the value map safely
     * TODO provide a method which takes a string view
     */
    void erase_value(const std::string &name);

    /**
     * @return return value with name, or nullptr
     * TODO provide a method which takes a string view
     */
    Value* find_value(const std::string& name);

    /**
     * return a value stored with name
     */
    inline Value* find(const chem::string_view& name) {
        return find_value(name.str());
    }

    /**
     * @return iterator for found value, the map that it was found in
     */
    std::pair<value_iterator, InterpretScope&> find_value_iterator(const std::string& name);

    /**
     * perform an operation between two values
     */
    Value* evaluate(Operation operation, Value* fEvl, Value* sEvl, SourceLocation location, Value* debugValue);

    /**
     * this node will be interpreted in this interpret scope
     */
    void interpret(ASTNode* node);

    /**
     * print all values
     */
    void print_values();

    /**
     * get if it's 64bit
     */
    inline constexpr bool isInterpret64Bit() {
        return sizeof(void*) == 8;
    }

    /**
     * emit an error for given node
     */
    void error(std::string& err, ASTNode* any);

    /**
     * emit an error for given value
     */
    void error(std::string& err, Value* any);

    /**
     *  emit an error for given node
     */
    void error(std::string_view err, ASTNode* any);

    /**
     * emit an error for given value
     */
    void error(std::string_view err, Value* any);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    virtual ~InterpretScope() = default;

};