// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "std/chem_string_view.h"
#include "ast/structures/VariantMemberParam.h"

enum class PatternElseExprKind {
    None,
    Unreachable,
    Return,
    Break,
    Continue,
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
     * For a tuple destructuring pattern (`var (a, b) = value`), the concrete
     * element type and position. Variant patterns leave these unset and use
     * `member_param` instead.
     */
    BaseType* bound_type = nullptr;
    uint32_t tuple_index = 0xFFFFFFFFu;
    /** Source index for a fixed-array destructuring pattern. */
    uint32_t array_index = 0xFFFFFFFFu;
    /** `_` discards this element; no local is declared or bound. */
    bool is_wildcard = false;

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

    PatternMatchIdentifier* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<PatternMatchIdentifier>()) PatternMatchIdentifier(
            matchExpr,
            identifier,
            parent(),
            encoded_location()
        );
        copied->member_param = member_param;
        copied->bound_type = bound_type;
        copied->tuple_index = tuple_index;
        copied->array_index = array_index;
        copied->is_wildcard = is_wildcard;
        return copied;
    }

    inline BaseType* known_type() {
        if (bound_type != nullptr) return bound_type;
        return member_param != nullptr
                   ? const_cast<BaseType*>(member_param->type.getType())
                   : nullptr;
    }


};

class PatternMatchExpr : public Value {
public:

    /**
     * is const
     */
    bool is_const;

    /**
     * the param names match the member param in the variant
     */
    bool destructure_by_name;

    /**
     * A tuple pattern binds positional elements of a tuple value rather than
     * a variant payload. Tuple patterns are irrefutable and have no `else`.
     */
    bool is_tuple = false;

    /**
     * A fixed-size array pattern (`var [a, b] = arr`). `array_rest_index`
     * is the pattern position of `..`, or -1 for an exact-length pattern.
     */
    bool is_array_pattern = false;
    int32_t array_rest_index = -1;

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
     * we set this during linking
     */
    VariantMember* member = nullptr;


    /**
     * constructor
     */
    PatternMatchExpr(
        bool is_const,
        bool destructure_by_name,
        chem::string_view member_name,
        SourceLocation location
    ) : Value(ValueKind::PatternMatchExpr, location), is_const(is_const),
        destructure_by_name(destructure_by_name), member_name(member_name), expression(nullptr)
    {

    }

    /**
     * constructor
     */
    PatternMatchExpr(
            bool is_const,
            chem::string_view member_name,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::PatternMatchExpr, type, location), is_const(is_const), member_name(member_name),
        expression(nullptr), destructure_by_name(false)
    {

    }

    VariantMember* find_member_from_expr(ASTAllocator& allocator, ASTDiagnoser& diagnoser);


    void copy_to_constructed(ASTAllocator& allocator, PatternMatchExpr* copied) {
        for(const auto name : param_names) {
            const auto id = name->copy(allocator);
            id->matchExpr = copied;
            copied->param_names.emplace_back(id);
        }
        copied->destructure_by_name = destructure_by_name;
        copied->is_tuple = is_tuple;
        copied->is_array_pattern = is_array_pattern;
        copied->array_rest_index = array_rest_index;
        copied->expression = expression->copy(allocator);
        auto& elseExpr = copied->elseExpression;
        elseExpr.kind = elseExpression.kind;
        if(elseExpression.value) {
            elseExpr.value = elseExpression.value->copy(allocator);
        }
        copied->member = member;
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<PatternMatchExpr>()) PatternMatchExpr(
            is_const, member_name, getType(), encoded_location()
        );
        copy_to_constructed(allocator, copied);
        return copied;
    }


};