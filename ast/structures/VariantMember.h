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

    tsl::ordered_map<std::string, VariantMemberParam*> values;
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

    BaseDefMember *copy_member(ASTAllocator& allocator) override;

    bool get_is_const() override {
        return true;
    }

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

    ASTNode *child(unsigned int index);

    BaseType* child_type(unsigned int index);

    bool requires_destructor();

    bool requires_clear_fn();

    bool requires_copy_fn();

    bool requires_move_fn();

    BaseType* known_type() override;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_value_type() override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};