// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "std/chem_string_view.h"

enum class PatternElseExprKind {
    None,
    Unreachable,
    Return,
    DefValue,

};

class PatternElseExpr {
public:
    /**
     * else expression kind
     */
    PatternElseExprKind kind = PatternElseExprKind::None;
    /**
     * if this is set, if kind is return, user aims to return it
     * if this is set, if kind is defValue, user aims to have it as a default value
     */
    Value* value = nullptr;
};

class PatternMatchIdentifier : public ASTNode {
public:

    /**
     * the pattern match expression
     */
    PatternMatchExpr* matchExpr;

    /**
     * the name of the identifier, that'll be destructured
     */
    chem::string_view identifier;

    /**
     * this is set by pattern match expression during linking
     */
    VariantMemberParam* member_param = nullptr;

    /**
     * constructor
     */
    PatternMatchIdentifier(
        PatternMatchExpr* matchExpr,
        chem::string_view identifier,
        ASTNode* parent_node,
        SourceLocation loc
    ) : ASTNode(ASTNodeKind::PatternMatchId, parent_node, loc), matchExpr(matchExpr), identifier(identifier) {

    }

};

class PatternMatchExpr : public Value {
public:

    /**
     * is const
     */
    bool is_const;

    /**
     * the member name inside the variant
     */
    chem::string_view member_name;
    /**
     * parameter names that user required to destructure
     */
    std::vector<PatternMatchIdentifier*> param_names;
    /**
     * the expression from which to destructure
     */
    Value* expression;
    /**
     * else expression for this pattern match
     */
    PatternElseExpr elseExpression;

    /**
     * constructor
     */
    PatternMatchExpr(
        bool is_const,
        chem::string_view member_name,
        SourceLocation location
    ) : Value(ValueKind::PatternMatchExpr, location), is_const(is_const), member_name(member_name),
        expression(nullptr)
    {

    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    BaseType* create_type(ASTAllocator &allocator) override;

};