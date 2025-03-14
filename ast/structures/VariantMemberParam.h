// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMember;

class VariantMemberParam : public ASTNode {
public:

    chem::string_view name;
    BaseType* type;
    Value* def_value;
    unsigned index;
    bool is_const;

    /**
     * constructor
     */
    constexpr VariantMemberParam(
        chem::string_view name,
        unsigned index,
        bool is_const,
        BaseType* type,
        Value* def_value,
        VariantMember* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::VariantMemberParam, (ASTNode*) parent_node, location), name(name), index(index),
        type(type), def_value(def_value), is_const(is_const) {

    }

    inline ASTNode* parent_node() {
        return ASTNode::parent();
    }

    inline VariantMember* parent() {
        return (VariantMember*) ASTNode::parent();
    }

    VariantMemberParam* copy(ASTAllocator& allocator) {
        return new (allocator.allocate<VariantMemberParam>()) VariantMemberParam(
                name, index, is_const, type->copy(allocator), def_value ? def_value->copy(allocator) : nullptr, parent(), encoded_location()
        );
    }

    void link_signature(SymbolResolver &linker) override;

    uint64_t byte_size(bool is64Bit) final {
        return type->byte_size(is64Bit);
    }

    BaseType* known_type() final {
        return type;
    }

    ASTNode* child(int index) final;

    int child_index(const chem::string_view &name) final;

    ASTNode* child(const chem::string_view &name) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};