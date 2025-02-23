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

    /**
     * constructor\
     */
    inline explicit ChainValue(ValueKind k) noexcept : Value(k) {

    }

    /**
     * called by access chain, to link a value inside values in a chain
     * it allows variable identifier to prevent auto appending self, when access chain has already done it
     */
    virtual bool link(
            SymbolResolver& linker,
            std::vector<ChainValue*>& values,
            unsigned index,
            BaseType* expected_type
    ) = 0;

    /**
     * relink with the parent chain value, as it has changed
     *
     * This allows re linking values when parent has changed due to copying of
     * access chain, which happens a lot in comptime functions
     */
    virtual void relink_parent(ChainValue* parent);

//    /**
//     * when value is part of access chain, this should be called
//     */
//    virtual std::unique_ptr<BaseType> create_type(std::vector<ChainValue*>& chain, unsigned int index);

    /**
     * an empty vector should be provided, so we can store all the generic iterations for the access chain
     * and restore them later
     * this function set's the generic iterations (what's that mean ?) ok, consider the chain p.a.b
     * in p.a.b, suppose the 'p' refers to a var declaration of a generic struct like var p = gen_struct<T>()
     * now to access a inside p we must set the type (iteration) of T (the generic parameter in parent struct) so that type of
     * a is concrete (if it depends on the type parameter), to access b we must set the generic iteration of a
     * because the struct of 'p' can contain a member like struct gen_struct<T> { var a : T }
     * so yeah this function goes ahead sets the generic iterations of p.a.b so types become concrete and access them
     * which means now if we call create_type on p.a.b we will get the exact type represented by that generic
     */
    void set_generic_iteration(std::vector<int16_t>& active_iterations, ASTAllocator& allocator);

    /**
     * this restore the generic iterations, this vector must be the same one that was given to the set_generic_iteration
     * this will take out iterations for each value in chain for example p.a.b and use the previously set iteration
     */
    void restore_generic_iteration(std::vector<int16_t>& active_iterations, ASTAllocator& allocator);

    /**
     * will check contents, are they of the same kind
     * this method means are both chain values functionally equal
     * they don't mean to check if they have same names, but if both were exchanged
     * they won't have an effect on code generation
     */
    bool is_equal(ChainValue* other, ValueKind this_kind, ValueKind other_kind);

    /**
     * a helper function
     */
    bool is_equal(ChainValue* other) {
        return is_equal(other, val_kind(), other->val_kind());
    }

#ifdef COMPILER_BUILD

    /**
     * called by access chain on the last ref value in the chain
     * by default it allocates chain->llvm_type and stores chain->llvm_value in it
     */
    virtual llvm::AllocaInst* access_chain_allocate(
            Codegen& gen,
            std::vector<ChainValue*>& values,
            unsigned int until,
            BaseType* expected_type
    );

    /**
     * so the parent pointer is returned, along with it's index
     * this is called by other access chain methods, to get the parent and then
     * load the child, this offers advantage like having access to parent value
     * like x.y.z <-- you get access to 'y' and then you load 'z' using GEP
     * now suppose functions that demand parent value, x.y.z() here z is a lambda
     * stored inside y, where y is supposed to be passed to lambda z (implicit self)
     * Please note that until index is the final value, not the parent index that you
     * want to retrieve
     */
    static std::pair<unsigned int, llvm::Value*> access_chain_parent_pointer(
            Codegen &gen,
            std::vector<ChainValue*>& values,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            unsigned int until,
            std::vector<llvm::Value*>& idxList
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
    llvm::Value* access_chain_pointer(
            Codegen &gen,
            std::vector<ChainValue*>& values,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            unsigned int until
    );

    /**
     * called by access chain on the last ref value in the chain
     * by default it just creates a load instruction on the access_chain_pointer by retrieving it from below
     *
     * this takes a vector destructibles which allows you to append objects to the destructibles, that will be destructed
     */
    llvm::Value* access_chain_value(
            Codegen &gen,
            std::vector<ChainValue*>& values,
            unsigned int until,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            BaseType* expected_type
    );

    /**
     * called by access chain on the last ref value in the chain
     * by default it just creates a load instruction on the access_chain_pointer by retrieving it from below
     *
     * this takes a vector destructibles which allows you to append objects to the destructibles, that will be destructed
     */
    virtual void access_chain_assign_value(
            Codegen &gen,
            AccessChain* chain,
            unsigned int until,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            llvm::Value* lhsPtr,
            Value* lhs,
            BaseType* expected_type
    );

    /**
     * helper function to call the actual access_chain_value
     */
    llvm::Value* access_chain_value(
            Codegen &gen,
            std::vector<ChainValue*>& values,
            unsigned int until,
            BaseType* expected_type
    ) {
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        auto value = access_chain_value(gen, values, until, destructibles, expected_type);
        destruct(gen, destructibles);
        return value;
    }

#endif

};