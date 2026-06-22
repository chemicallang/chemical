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

using node_map = std::unordered_map<chem::string_view, ASTNode*>;
using node_iterator = node_map::iterator;
using value_map = std::unordered_map<chem::string_view, Value*>;
using value_iterator = value_map::iterator;

class FunctionType;

class InterpretScope {
public:

    /**
      * This contains a map between identifiers and its values, of the current scope
      */
    std::unordered_map<chem::string_view, Value*> values;

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
     * get the null value
     */
    Value* getNullValue();

    /**
     * declares a value with this name in current scope
     */
    void declare(const chem::string_view& name, Value* value);

    /**
     * erases a value by the key name from the value map safely
     */
    void erase_value(const chem::string_view& name);

    /**
     * return value with name, or nullptr
     */
    Value* find_value(const chem::string_view& name);

    /**
     * @return iterator for found value, the map that it was found in
     */
    std::pair<value_iterator, InterpretScope&> find_value_iterator(const chem::string_view& name);

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
     * Return value for the current function being interpreted.
     * This is set by set_return() and retrieved by call() after
     * body interpretation completes. Stored here instead of on
     * AST nodes to avoid storing comptime state in the AST.
     */
    Value* returnValue = nullptr;

    /**
     * Implicit arguments map — populated by `provide` statements.
     * When a function with implicit parameters is called, the interpreter
     * looks up the parameter name in this map from the caller's scope.
     */
    std::unordered_map<chem::string_view, Value*> implicit_args;

    /**
     * When set to false (e.g. global scope), values in this scope
     * are NOT destructed when the scope ends.
     */
    bool should_destruct_values = true;

    /**
     * When a return statement is interpreted inside this scope,
     * this flag is set to stop further interpretation of sibling
     * nodes in all parent scopes up to the function scope.
     * Prevents non-loop scopes (like if-blocks) from continuing
     * after a return has been processed.
     */
    bool stopInterpretation = false;

    /**
     * Iterates over all values in this scope and calls destructors
     * for struct values that have destructor functions defined.
     * The returnValue is skipped (it has been moved to the caller).
     */
    void destroy_values();

    /**
     * Move semantics helper: after declaring a new variable that holds a destructible
     * struct, scan the scope chain for any existing variable pointing to the same
     * StructValue pointer and clear it (set to nullptr). This prevents double-
     * destruction when a struct is moved from one variable to another.
     * Works correctly regardless of whether the AST node is a VariableIdentifier
     * or has been replaced by the compiler during resolution.
     */
    void move_clear_source(Value* initializer, const chem::string_view& new_name);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in this destructor
     */
    virtual ~InterpretScope();

};