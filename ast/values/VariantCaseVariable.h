// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class VariantMemberParam;

class VariantCaseVariable : public ASTNode {
public:

    chem::string_view name;
    VariantMemberParam* member_param;

    /**
     * variant case
     */
    constexpr VariantCaseVariable(
            chem::string_view name,
            VariantMemberParam* member_param,
            SwitchStatement* switch_statement,
            SourceLocation loc
    ) : ASTNode(ASTNodeKind::VariantCaseVariable, (ASTNode*) switch_statement, loc), name(name), member_param(member_param) {

    }

    inline ASTNode* parent_node() {
        return ASTNode::parent();
    }

    inline SwitchStatement* parent() {
        return (SwitchStatement*) ASTNode::parent();
    }

    BaseType* known_type() final;

    bool is_generic_param();

    VariantCaseVariable* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<VariantCaseVariable>()) VariantCaseVariable(
            name, member_param, parent(), encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) final;

    llvm::Value* llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};