// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/Value.h"

/**
 * when user writes an if as a type
 * type X = if(condition) type1 else type2
 * only can be written in type aliases (statements)
 * because it requires special treatment
 *
 * type X<T> = if(condition) some<T> else other<T>
 *
 *
 */
class IfType : public BaseType {
public:

    Value* condition;
    TypeLoc thenType;
    std::vector<std::pair<Value*, TypeLoc>> elseIfs;
    TypeLoc elseType;

    /**
     * constructor
     */
    inline constexpr IfType(
        Value* condition,
        TypeLoc thenType,
        TypeLoc elseType
    ) : BaseType(BaseTypeKind::IfType), condition(condition), thenType(thenType), elseType(elseType) {

    }

    /**
     * if type only exists in type aliases, which evaluates if type specially
     * preventing any calls to is_same
     */
    bool is_same(BaseType *type) override {
#ifdef DEBUG
        throw std::runtime_error("comparison of if type is prohibited");
#endif
        return false;
    }

    /**
     * if type only exists in type aliases, which evaluates if type specially
     * preventing any calls to satisfies
     */
    bool satisfies(BaseType *type) override {
#ifdef DEBUG
        throw std::runtime_error("checking satisfaction on a if type is prohibited");
#endif
        return false;
    }

    /**
     * this if type can be evaluated to get the type
     * note: this can return null if condition couldn't be resolved
     */
    TypeLoc evaluate(InterpretScope& scope);

    /**
     * copy the if type
     */
    BaseType* copy(ASTAllocator &allocator) override {
        const auto other = new (allocator.allocate<IfType>()) IfType(
            condition->copy(allocator),
            thenType.copy(allocator),
            elseType.copy(allocator)
        );
        for(auto& pair : elseIfs) {
            other->elseIfs.emplace_back(pair.first->copy(allocator), pair.second.copy(allocator));
        }
        return other;
    }

};