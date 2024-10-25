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
    SourceLocation location;

    VariantMember(
            const std::string& name,
            VariantDefinition* parent_node,
            SourceLocation location
    );

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::VariantMember;
    }

    uint64_t byte_size(bool is64Bit) final {
        uint64_t total_bytes = 0;
        for(auto& value : values) {
            total_bytes += value.second->byte_size(is64Bit);
        }
        return total_bytes;
    }

    ASTNode *parent() final {
        return (ASTNode*) parent_node;
    }

    BaseDefMember *copy_member(ASTAllocator& allocator) final;

    bool get_is_const() final {
        return true;
    }

    void accept(Visitor *visitor) final;

    void declare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *child(const std::string &name) final;

    ASTNode *child(unsigned int index);

    BaseType* child_type(unsigned int index);

    bool requires_destructor();

    bool requires_clear_fn();

    bool requires_copy_fn();

    bool requires_move_fn();

    BaseType* known_type() final;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};