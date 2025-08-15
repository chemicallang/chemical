// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/PatternMatchExpr.h"

class PatternMatchExprNode : public ASTNode {
public:

    PatternMatchExpr value;

    PatternMatchExprNode(
        bool is_const,
        chem::string_view member_name,
        BaseType* type,
        SourceLocation loc,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::PatternMatchExprNode, parent_node, loc), value(is_const, member_name, type, loc) {

    }

    PatternMatchExprNode(
            bool is_const,
            bool destructure_by_name,
            chem::string_view member_name,
            SourceLocation loc,
            ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::PatternMatchExprNode, parent_node, loc), value(is_const, destructure_by_name, member_name, loc) {

    }

    PatternMatchExprNode* copy(ASTAllocator &allocator) override {
        const auto node = new (allocator.allocate<PatternMatchExprNode>()) PatternMatchExprNode(
            value.is_const, value.member_name, value.getType(),
            encoded_location(),
            parent()
        );
        value.copy_to_constructed(allocator, &node->value);
        return node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = chain.llvm_value(gen);
        chain.llvm_destruct(gen, inst);
    }

#endif

};