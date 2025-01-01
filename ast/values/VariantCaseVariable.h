// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMemberParam;

class VariantCaseVariable : public ASTNode {
public:

    std::string name;
    VariableIdentifier* parent_val;
    VariantMemberParam* member_param;
    SwitchStatement* switch_statement;
    SourceLocation location;

    /**
     * variant case
     */
    VariantCaseVariable(
            std::string name,
            VariableIdentifier* parent_val,
            SwitchStatement* switch_statement,
            SourceLocation token
    );

    SourceLocation encoded_location() final {
        return location;
    }

    void accept(Visitor *visitor) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNodeKind kind() final {
        return ASTNodeKind::VariantCaseVariable;
    }

    ASTNode* parent() final {
        return (ASTNode*) switch_statement;
    }

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    ASTNode* child(const std::string &name) final;

    int child_index(const std::string &name) final;

    ASTNode* child(int index) final;

    std::pair<BaseType*, int16_t> set_iteration();

    bool is_generic_param();

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) final;

    llvm::Value* llvm_load(Codegen &gen) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

#endif

};