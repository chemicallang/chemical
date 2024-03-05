// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ASTNode.h"
#include "InterpretValue.h"
#include "ValueType.h"

/**
 * @brief Base class for all values in the AST.
 */
class Value : public ASTNode, public InterpretValue {
public:

    std::string interpret_representation() const override {
        return representation();
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
    virtual Value* evaluated_value(scope_vars &scopeVars) {
        return this;
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
     * a function to be overridden by int values to return actual values
     * @return
     */
    virtual int as_int() {
        throw std::runtime_error("as_int called on a value");
    }

    /**
     * evaluate the value to another value
     * for example logical expressions can be evaluated to booleans
     * @return
     */
    virtual std::unique_ptr<Value> evaluate() {
        throw std::runtime_error("evaluating a base value is not possible");
    }

    /**
     * returns the type of value
     * @return
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    };

};