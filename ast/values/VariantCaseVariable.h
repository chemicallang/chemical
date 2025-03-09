// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class VariantMemberParam;

class VariantCaseVariable : public ASTNode {
public:

    chem::string_view name;
    VariableIdentifier* parent_val;
    VariantMemberParam* member_param;
    SwitchStatement* switch_statement;

    /**
     * variant case
     */
    constexpr VariantCaseVariable(
            chem::string_view name,
            VariableIdentifier* parent_val,
            SwitchStatement* switch_statement,
            SourceLocation token
    ) : ASTNode(ASTNodeKind::VariantCaseVariable, (ASTNode*) switch_statement, parent_val->encoded_location()), name(name), parent_val(parent_val), switch_statement(switch_statement) {

    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    inline ASTNode* parent_node() {
        return ASTNode::parent();
    }

    inline SwitchStatement* parent() {
        return (SwitchStatement*) ASTNode::parent();
    }

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    ASTNode* child(const chem::string_view &name) final;

    int child_index(const chem::string_view &name) final;

    ASTNode* child(int index) final;

    bool is_generic_param();

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer_no_itr(Codegen& gen);

    llvm::Value* llvm_pointer(Codegen &gen) final;

    llvm::Value* llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};