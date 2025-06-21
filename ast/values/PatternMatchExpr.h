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

    PatternMatchIdentifier* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<PatternMatchIdentifier>()) PatternMatchIdentifier(
            matchExpr,
            identifier,
            parent(),
            encoded_location()
        );
        copied->member_param = member_param;
        return copied;
    }

    BaseType* known_type() override;

    ASTNode* child(const chem::string_view &name) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_load(Codegen &gen, SourceLocation location) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

#endif

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
     * we set this during linking
     */
    VariantMember* member = nullptr;

#ifdef COMPILER_BUILD

    /**
     * this is evaluated in llvm_value
     */
    llvm::Value* llvm_expr = nullptr;

#endif

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

    VariantMember* find_member_from_expr(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<PatternMatchExpr>()) PatternMatchExpr(
            is_const, member_name, encoded_location()
        );
        for(const auto name : param_names) {
            const auto id = name->copy(allocator);
            id->matchExpr = copied;
            copied->param_names.emplace_back(id);
        }
        copied->expression = expression->copy(allocator);
        auto& elseExpr = copied->elseExpression;
        elseExpr.kind = elseExpression.kind;
        if(elseExpression.value) {
            elseExpr.value = elseExpression.value->copy(allocator);
        }
        copied->member = member;
        return copied;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    void llvm_conditional_branch(Codegen &gen, llvm::BasicBlock *then_block, llvm::BasicBlock *otherwise_block) override;

#endif

};