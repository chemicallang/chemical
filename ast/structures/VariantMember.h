// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"
#include "ast/types/LinkedType.h"
#include "VariantMemberParam.h"

struct VariantMemberAttributes {

    bool deprecated;

};

class VariantMember : public BaseDefMember {
public:

    tsl::ordered_map<chem::string_view, VariantMemberParam*> values;
    LinkedType ref_type;
    VariantMemberAttributes attrs;

    /**
     * constructor
     */
    VariantMember(
            chem::string_view name,
            VariantDefinition* parent_node,
            SourceLocation location
    ) : BaseDefMember(name, ASTNodeKind::VariantMember, (ASTNode*) parent_node, location),
        ref_type(name, this, location), attrs(false)
    {

    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    uint64_t byte_size(bool is64Bit) final {
        uint64_t total_bytes = 0;
        for(auto& value : values) {
            total_bytes += value.second->byte_size(is64Bit);
        }
        return total_bytes;
    }

    inline ASTNode* parent_node() {
        return ASTNode::parent();
    }

    inline VariantDefinition* parent() {
        return (VariantDefinition*) ASTNode::parent();
    }

    VariantMember *copy_member(ASTAllocator& allocator) final {
        const auto member = new (allocator.allocate<VariantMember>()) VariantMember(name, parent(), encoded_location());
        for(auto& value : values) {
            const auto param_copy = value.second->copy(allocator);
            param_copy->set_parent(member);
            member->values[value.first] = param_copy;
        }
        member->attrs = attrs;
        return member;
    }

    bool get_is_const() final {
        return true;
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    ASTNode *child(const chem::string_view &name) final;

    ASTNode *child(unsigned int index);

    BaseType* child_type(unsigned int index);

    bool requires_destructor();

    bool requires_clear_fn();

    bool requires_copy_fn();

    bool requires_move_fn();

    BaseType* known_type() final;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    // not wrapped, basically doesn't contain the type integer
    // it's just a struct of all the variant parameters user defined
    llvm::Type* llvm_raw_struct_type(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};