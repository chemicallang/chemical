// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "Interpretable.h"
#include "ValueType.h"
#include "ast/utils/Operation.h"
#include "compiler/Codegen.h"
#ifdef COMPILER_BUILD
#include "llvm/IR/Value.h"
#endif

class FunctionDeclaration;

class InterfaceDefinition;

/**
 * @brief Base class for all values in the AST.
 */
class Value : public Interpretable {
public:

    /**
     * if this value has a child by this name, it should return a pointer to it
     * @param name
     * @return
     */
    virtual Value* child(const std::string& name) {
#ifdef DEBUG
std::cerr << "child called on base value";
#endif
        return nullptr;
    }

    /**
     * index operator [] calls this on a value
     */
    virtual Value* index(int i) {
#ifdef DEBUG
        std::cerr << "index called on base value";
#endif
        return nullptr;
    }

    /**
     * set the child value, with given name, performing operation op
     */
    virtual void set_child_value(const std::string& name, Value* value, Operation op) {
#ifdef DEBUG
        std::cerr << "set_child_value called on base value";
#endif
    }

    /**
     * this function expects this identifier value to find itself in the parent value given to it
     * this is only expected to be overridden by identifiers
     * @param parent
     * @return
     */
    virtual Value* find_in(Value* parent) {
#ifdef DEBUG
        std::cerr << "find_in called on base value";
#endif
       return nullptr;
    }

    /**
     * called on a identifier to set it's value in the given parent
     */
    virtual void set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op) {
        scope.error("set_value_in called on base value");
    }

    /**
     * give representation of the value as it appears in source
     * this doesn't need to be 100% accurate
     * @return
     */
    virtual std::string representation() const {
        return "[Value:Base:Representation]";
    }

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string interpret_representation() const {
        return representation();
    };

#ifdef COMPILER_BUILD
    /**
     * code_gen function that generates llvm Value
     * @return
     */
    virtual void code_gen(Codegen& gen) {
        throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation() + " , type : " + std::to_string((unsigned int) value_type()));
    }

    /**
     * provides llvm_type for the given value
     * @param gen
     * @return
     */
    virtual llvm::Type* llvm_type(Codegen& gen) {
        throw std::runtime_error("llvm_type called on bare Value of type " + std::to_string((int) value_type()));
    };

    /**
     * allocates this value with this identifier, and also creates a store instruction
     * @param gen
     * @param identifier
     */
    virtual void llvm_allocate(Codegen& gen, const std::string& identifier) {
        auto x = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
        gen.allocated[identifier] = x;
        gen.builder->CreateStore(llvm_value(gen), x);
    }

    /**
     * provides llvm_elem_type, which is the child type for example elem type of an array value
     * @param gen
     * @return
     */
    virtual llvm::Type* llvm_elem_type(Codegen& gen) {
        throw std::runtime_error("llvm_elem_type called on bare Value of type " + std::to_string((int) value_type()));
    };

    /**
     * returns a llvm pointer to the value, if has any
     * @param gen
     * @return
     */
    virtual llvm::Value* llvm_pointer(Codegen& gen) {
        throw std::runtime_error("llvm_pointer called on bare Value of type " + std::to_string((int) value_type()));
    }

    /**
     * creates and returns the llvm value
     * @return
     */
    virtual llvm::Value* llvm_value(Codegen& gen) {
        throw std::runtime_error("llvm_value called on bare Value with representation : " + representation() + " , type " + std::to_string((int) value_type()));
    }

    /**
     * creates the value, casts it to its type and returns it
     * @param gen
     * @return
     */
    virtual llvm::Value* casted_llvm_value(Codegen &gen) {
        throw std::runtime_error("casted_llvm_value called on bare Value of type " + std::to_string((int) value_type()));
    }
#endif

    /**
     * Called by assignment statement, to set the value of this value
     * since InterpretValue can also represent an identifier or access chain
     * This method is overridden by for ex : an identifier, which can find itself in the scope above and set this value
     * The given op must be performed on the already stored value
     * @param vars
     * @param value
     * @param op
     */
    virtual void set_identifier_value(InterpretScope& scope, Value* value, Operation op) {
        scope.error("set_identifier_value called on base value");
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
    virtual Value* copy() const {
        return nullptr;
    };

    /**
     * This method is called by the ASTNode to get an initializer value
     * an initializer value is basically value that will initialize a variable
     * usually values copy themselves over, expressions evaluate themselves
     * because in an AST a value is basically a fragment of AST, but when interpreting
     * it becomes an actual value that can be modified, for any modifications values must be copied
     * @return
     */
    virtual Value* initializer_value(InterpretScope& scope) {
        return copy();
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
     * does this value compute the value, in other words (is it an expression -> e.g a + b)
     * @return
     */
    virtual bool computed() {
        return false;
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
     * a function to be overridden by char values to return actual values
     */
    virtual char as_char() {
#ifdef DEBUG
        std::cerr << "as_char called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_char called on a value");
    }

    /**
     * a function to be overridden by bool values to return actual values
     * @return
     */
    virtual bool as_bool() {
#ifdef DEBUG
        std::cerr << "as_bool called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_bool called on a value");
    }

    /**
     * a function to be overridden by values that can return string
     * @return
     */
    virtual std::string as_string() {
#ifdef DEBUG
        std::cerr << "as_string called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_string called on a value");
    }

    /**
     * a function to be overridden by values that can return int
     * @return
     */
    virtual int as_int() {
#ifdef DEBUG
        std::cerr << "as_int called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_int called on a value");
    }

    /**
     * a function to be overridden by values that can return float
     * @return
     */
    virtual float as_float() {
#ifdef DEBUG
        std::cerr << "as_float called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_float called on a value");
    }

    /**
     * a function to be overridden by values that can return double
     * @return
     */
    virtual double as_double() {
#ifdef DEBUG
        std::cerr << "as_double called on base value, representation : " << representation();
#endif
        throw std::runtime_error("as_float called on a value");
    }

    /**
     * returns the type of value
     * @return
     */
    virtual ValueType value_type() const {
        return ValueType::Unknown;
    };

};