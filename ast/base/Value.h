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

    virtual std::string* getScopeIdentifier() {
        return nullptr;
    }

    virtual Value* getChild(std::string* name) {
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
     * this function is called with the current scope variables on the first element of the access chain
     * this allows locating the first element of the access chain in the scope above
     * once located, we can travel that variable to locate the rest of the access chain and use that value
     * @param scopeVars
     * @return
     */
    virtual Value * find_in_parent(InterpretScope& scope) {
        return nullptr;
    }

    /**
     * This method is overridden by primitive values like int, float... to return true
     * @return true when the value is primitive
     */
    virtual bool primitive() {
        return false;
    }

    /**
     * This returns how many references are to this value
     * All primitive values have only a single reference to its value, because on new variable creation
     * primitive values are copied into the variables
     * For complex types like struct, a references unsigned int is held to determine the number of references the struct has
     * A value is deleted when the scope ends if it only has a single reference, which is in the variable currently on the scope
     * @return
     */
    virtual unsigned int references() {
        return 1;
    }

    /**
     * This function is called by the scope, if this function has more than one reference and it couldn't be deleted
     * To allow the value to be deleted, we must decreases references count as we are deleting references
     * So when the last reference is in the scope, the references count = 1, so value can be safely deleted
     */
    virtual void decrease_reference() {

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

    virtual FunctionDeclaration* as_function() {
        std::cerr << "actual_type:" << std::to_string((int) value_type()) << std::endl;
        throw std::runtime_error("as_function called on a value");
    }

    /**
     * a function to be overridden by int values to return actual values
     * @return
     */
    virtual int as_int() {
        throw std::runtime_error("as_int called on a value");
    }

    virtual float as_float() {
        throw std::runtime_error("as_float called on a value");
    }

    /**
     * This is called by for example, assignment statement, to locate the access chain completely
     * in the scope variables given, so that lhs value can be set
     * It is also called by the function for its parameters to locate them
     * @param scopeVars
     * @return
     */
    virtual Value* travel(InterpretScope& scope) {
        return nullptr;
    }

    /**
     * returns the type of value
     * @return
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    };

};