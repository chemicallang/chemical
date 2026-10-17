// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include <unordered_map>

struct AccessChainAttributes {

    bool is_moved;

};

/**
 * access chain represents a way to access things in programming language for example
 * x.y.z is an access chain, z member of the y of the x is being accessed
 * x.y.z() where z is a function call, z function is assumed to be present in y and y in z
 * z.y.z[0] similarly z is an index operator here
 */
class AccessChain : public Value {
public:

    std::vector<Value*> values;
    AccessChainAttributes attrs;

    /**
     * constructor
     */
    constexpr AccessChain(
        SourceLocation location
    ) : Value(ValueKind::AccessChain, location), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
            BaseType* chain_type,
            SourceLocation location
    ) : Value(ValueKind::AccessChain, chain_type, location), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
        std::vector<Value*> values,
        SourceLocation location
    ) : Value(ValueKind::AccessChain, location), values(std::move(values)), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
            std::vector<Value*> values,
            BaseType* chain_type,
            SourceLocation location
    ) : Value(ValueKind::AccessChain, chain_type, location), values(std::move(values)), attrs(false) {

    }

    inline bool is_moved() {
        return attrs.is_moved;
    }

    inline void set_is_moved(bool is_moved) {
        attrs.is_moved = is_moved;
    }

    /**
     * will call relink_parent on values starting from second value
     */
    void relink_parent();

    AccessChain *copy(ASTAllocator& allocator) final;

    bool primitive() final;

    bool compile_time_computable() final;

    uint64_t byte_size(const TargetData& target) final;


    Value* evaluated_value(InterpretScope &scope);


    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() final;

};

void copy_from(ASTAllocator& allocator, std::vector<Value*>& destination, std::vector<Value*>& source, unsigned from);

Value* evaluate_from(std::vector<Value*>& values, InterpretScope& scope, Value* evaluated, unsigned i);
