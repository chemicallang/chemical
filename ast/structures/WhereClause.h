// Copyright (c) Chemical Language Foundation 2025.
//
// Created by Buffy on 17/06/2026.
//

#pragma once

#include "ast/base/TypeLoc.h"
#include "ast/base/ASTAny.h"
#include "ast/base/ASTAllocator.h"
#include <vector>

// Forward declaration
class GenericTypeParameter;

/**
 * A single constraint in a `where` clause, e.g. `T: Copy + SomeTrait`
 */
struct WhereConstraint {
    /** The name of the generic type parameter being constrained */
    chem::string_view param_name;

    /** Resolved pointer to the GenericTypeParameter node (set during symbol resolution) */
    GenericTypeParameter* param = nullptr;

    /** The trait/interface types the parameter must satisfy */
    std::vector<TypeLoc> constraints;

    WhereConstraint(chem::string_view param_name, std::vector<TypeLoc> constraints)
        : param_name(param_name), constraints(std::move(constraints)) {}
};

/**
 * A `where` clause on a function declaration, e.g. `where T: Copy, K: SomeTrait`.
 *
 * Allocated on the ASTAllocator and stored as a pointer on FunctionDeclaration
 * so `if (where_clause != nullptr)` is a fast check and no extra space is wasted
 * for functions without a where clause.
 */
class WhereClause : public ASTAny {
public:
    std::vector<WhereConstraint> constraints;

    explicit WhereClause(std::vector<WhereConstraint> constraints)
        : constraints(std::move(constraints)) {}

    bool empty() const { return constraints.empty(); }

    ASTAnyKind any_kind() final { return ASTAnyKind::Node; }
};
