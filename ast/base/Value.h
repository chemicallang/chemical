// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ASTNode.h"
#include "ValueType.h"
#include "ast/utils/Operation.h"

class FunctionDeclaration;

/**
 * @brief Base class for all values in the AST.
 */
class Value : public ASTNode {
public:

    /**
     * if this value has a child by this name, it should return a pointer to it
     * @param name
     * @return
     */
    virtual Value* child(const std::string& name) {
        return nullptr;
    }

    /**
     * this function expects this identifier value to find itself in the parent value given to it
     * this is only expected to be overridden by identifiers
     * @param parent
     * @return
     */
    virtual Value* find_in(Value* parent) {
       return nullptr;
    }

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string interpret_representation() const {
        return representation();
    };

    /**
     * Called by assignment statement, to set the value of this value
     * since InterpretValue can also represent an identifier or access chain
     * This method is overridden by for ex : an identifier, which can find itself in the scope above and set this value
     * @param vars
     * @param value
     */
    virtual void set_identifier_value(InterpretScope& scope, Value* value) {

    }

    /**
     * Called by assignment statement, to set the value of this value
     * Operation is given to perform operation on the existing value
     * @param vars
     * @param value
     * @param op
     */
    virtual void set_identifier_value(InterpretScope& scope, Value* value, Operation op) {

    }

    /**
     * This method is overridden by primitive values like int, float... to return true
     * @return true when the value is primitive
     */
    virtual bool primitive() {
        return false;
    }

    /**
     * This method allows to make a copy of the current value
     * This method can only be called on primitive values as they are the only ones that support copy operation
     * @return
     */
    virtual Value* copy() {
        std::cerr << "[Value] copy operation called on base class";
        return this;
    }

    /**
     * This method should return the value for interpretation
     * By default it returns null pointer
     * @return
     */
    virtual void* get_value() {
        return nullptr;
    }

    /**
     * This returns Value pointer to the object that represents the real value
     * This can be used to get the value after evaluation
     * for ex : identifier represents a variable that contains a value, its not a value itself, but yields a value
     * so it can find its value and return pointer to the Value object that actually holds the value
     * @return
     */
    virtual Value* evaluated_value(InterpretScope& scope) {
        return this;
    }

    /**
     * This method is a helper method that evaluates the current value as a boolean
     * The only difference between this and as_bool is that when this is called
     * Not only bool is returned, the computations performed inside this value is deleted
     * @return
     */
    virtual bool evaluated_bool(InterpretScope& scope) {
        return this->evaluated_value(scope)->as_bool();
    }

    /**
     * finds the given identifier in the scope above, returns a pair, representing first value as true, if found
     * second value is an iterator to modify or access the value
     * @param scope
     * @param value
     * @return
     */
    std::pair<bool, std::unordered_map<std::string, Value *>::iterator> find(InterpretScope &scope, const std::string& value) {
        // try to find the pointer of the value
        auto currentScope = &scope;
        while (currentScope != nullptr) {
            auto pointer = currentScope->values.find(value);
            if (pointer != currentScope->values.end()) {
                return std::pair(true, std::move(pointer));
            }
            currentScope = currentScope->parent;
        }
        return std::pair(false, scope.values.end());
    }

    /**
     * a function to be overridden by bool values to return actual values
     * @return
     */
    virtual bool as_bool() {
        std::cerr << "actual_type:" << std::to_string((int) value_type()) << std::endl;
        throw std::runtime_error("as_bool called on a value");
    }

    /**
     * this is overridden by function declaration to return itself
     * @return
     */
    virtual FunctionDeclaration* as_function() {
        std::cerr << "actual_type:" << std::to_string((int) value_type()) << std::endl;
        throw std::runtime_error("as_function called on a value");
    }

    /**
     * a function to be overridden by values that can return int
     * @return
     */
    virtual int as_int() {
        throw std::runtime_error("as_int called on a value");
    }

    /**
     * a function to be overridden by values that can return float
     * @return
     */
    virtual float as_float() {
        throw std::runtime_error("as_float called on a value");
    }

    /**
     * returns the type of value
     * @return
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    };

    /**
     * called when the interpret scope ends, the interpret scope is the parent of the current value
     * By default deletes the current value
     */
    virtual void scope_ends() {
        delete this;
    }

};