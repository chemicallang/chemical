// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include <vector>

struct AsmOperand {
    chem::string_view constraint;
    Value* expr;

    AsmOperand() : expr(nullptr) {}
    AsmOperand(chem::string_view constraint, Value* expr) : constraint(constraint), expr(expr) {}
};

class InlineAsmStatement : public ASTNode {
public:

    chem::string_view asm_template;
    std::vector<AsmOperand> output_operands;
    std::vector<AsmOperand> input_operands;
    std::vector<chem::string_view> clobbers;

    InlineAsmStatement(
            chem::string_view asm_template,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::InlineAsmStmt, parent_node, location), asm_template(asm_template) {

    }

    InlineAsmStatement* copy(ASTAllocator &allocator) override {
        auto stmt = new (allocator.allocate<InlineAsmStatement>()) InlineAsmStatement(
            asm_template,
            parent(),
            encoded_location()
        );
        for(auto& op : output_operands) {
            stmt->output_operands.emplace_back(op.constraint, op.expr ? op.expr->copy(allocator) : nullptr);
        }
        for(auto& op : input_operands) {
            stmt->input_operands.emplace_back(op.constraint, op.expr ? op.expr->copy(allocator) : nullptr);
        }
        for(auto& c : clobbers) {
            stmt->clobbers.emplace_back(c);
        }
        return stmt;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

#endif

};
