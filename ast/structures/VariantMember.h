// Copyright (c) Qinetik 2024.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"
#include "ast/types/LinkedType.h"
#include "VariantMemberParam.h"

class VariantMember : public BaseDefMember {
public:

    tsl::ordered_map<std::string, std::unique_ptr<VariantMemberParam>> values;
    VariantDefinition* parent_node;
    LinkedType ref_type;
    CSTToken* token;

    VariantMember(
            const std::string& name,
            VariantDefinition* parent_node,
            CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::VariantMember;
    }

    uint64_t byte_size(bool is64Bit) override {
        uint64_t total_bytes = 0;
        for(auto& value : values) {
            total_bytes += value.second->byte_size(is64Bit);
        }
        return total_bytes;
    }

    ASTNode *parent() override {
        return (ASTNode*) parent_node;
    }

    BaseDefMember *copy_member() override;

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNode *child(const std::string &name) override;

    ASTNode *child(unsigned int index);

    BaseType* child_type(unsigned int index);

    bool requires_destructor() override;

    bool requires_clear_fn() override;

    bool requires_copy_fn() override;

    BaseType* known_type() override;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};