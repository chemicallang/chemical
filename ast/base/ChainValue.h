// Copyright (c) Qinetik 2024.

#pragma once

#include "Value.h"

/**
 * chain value should be inherited by values that want to appear in a access chain
 * like x.y.z
 */
class ChainValue : public Value {
public:

    /**
     * usages of the base class method, so we don't have to type Value:: everywhere
     */
    using Value::create_type;
    using Value::link;

    ChainValue * as_chain_value() override {
        return this;
    }

    /**
     * called by access chain, to link a value inside values in a chain
     * it allows variable identifier to prevent auto appending self, when access chain has already done it
     */
    virtual void link(
            SymbolResolver& linker,
            ChainValue* parent,
            std::vector<std::unique_ptr<ChainValue>>& values,
            unsigned index
    );

    /**
     * find linked node in given parent node, symbol resolver is passed in resolution phase
     */
    virtual void find_link_in_parent(ChainValue* parent, SymbolResolver& resolver) = 0;

    /**
     * relink with the parent chain value, as it has changed
     *
     * This allows re linking values when parent has changed due to copying of
     * access chain, which happens a lot in comptime functions
     */
    virtual void relink_parent(ChainValue* parent);

    /**
     * when value is part of access chain, this should be called
     */
    virtual std::unique_ptr<BaseType> create_type(std::vector<std::unique_ptr<ChainValue>>& chain, unsigned int index);

    /**
     * if this chain value is connected to a type that has a generic iteration
     * this will set it, also returns the previous generic iteration no the struct
     * otherwise -2
     */
    int16_t set_generic_iteration();

#ifdef COMPILER_BUILD

    /**
     * called by access chain on the last ref value in the chain
     * by default it allocates chain->llvm_type and stores chain->llvm_value in it
     */
    virtual llvm::AllocaInst* access_chain_allocate(
            Codegen& gen,
            std::vector<std::unique_ptr<ChainValue>>& values,
            unsigned int until
    );

    /**
     * when a identifier is last in the access chain, for example x.y.z here z is the last identifier
     * this function will be called on it, the values given are the values of access chain
     * by default this just calls default_chain_pointer
     * @param values part of access chain, identifier / function call / index operator
     * @param until the values are considered up until this (exclusive)
     *
     * @param destructibles values allocated inside access chain, for example when x.y().z where y returns a struct which has z as a member
     * y call means -> allocate a struct, and pass pointer to y, after accessing and loading z, destruct struct created by y
     * to destruct the struct, destructibles vector is used to keep track of values inside this chain that must be destructed after loading
     * since this just returns a pointer to it, and doesn't load the value, the value must be preserved till loaded and not destructed.
     * after pointer received by this function call has been loaded, destructibles vector should be destructed in reverse order
     *
     */
    virtual llvm::Value* access_chain_pointer(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>>& values,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            unsigned int until
    );

    /**
     * called by access chain on the last ref value in the chain
     * by default it just creates a load instruction on the access_chain_pointer by retrieving it from below
     *
     * this takes a vector destructibles which allows you to append objects to the destructibles, that will be destructed
     */
    virtual llvm::Value* access_chain_value(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>>& values,
            unsigned int until,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles
    );

    /**
     * helper function to call the actual access_chain_value
     */
    llvm::Value* access_chain_value(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>>& values,
            unsigned int until
    ) {
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        auto value = access_chain_value(gen, values, until, destructibles);
        destruct(gen, destructibles);
        return value;
    }

#endif

};