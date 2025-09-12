// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

struct EnumMemberAttributes {
    bool deprecated;
};

class EnumMember : public ASTNode {
private:

    /**
     * this either is the default index we used (from zero to onwards) or user specified
     * if the enum inherits another enum then this index can change (starts from where base enum left off)
     */
    int index;

public:

    chem::string_view name;
    /**
     * this init_value may be nullptr, if user didn't specify an explicit value for
     * this enum member
     */
    Value* init_value;
    /**
     * attributes for the enum member
     */
    EnumMemberAttributes attrs;

    /**
     * constructor
     */
    constexpr EnumMember(
        chem::string_view name,
        int index,
        Value* init_value,
        EnumDeclaration* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::EnumMember, (ASTNode*) parent_node, location), name(name), index(index),
        init_value(init_value), attrs(false)
    {

    }

    EnumMember* copy(ASTAllocator &allocator) override {
        const auto member = new (allocator.allocate<EnumMember>()) EnumMember(
            name,
            index,
            init_value ? init_value->copy(allocator) : nullptr,
            parent(),
            encoded_location()
        );
        member->attrs = attrs;
        return member;
    }

    inline EnumDeclaration* parent() {
        return (EnumDeclaration*) ASTNode::parent();
    }

    inline ASTNode* parent_node() {
        return (ASTNode*) ASTNode::parent();
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    /**
     * the default index we'll use for this enum member, if user didn't specify an
     * explicit index for it
     */
    inline int get_default_index() {
        return index;
    }

    /**
     * get the index dirty
     */
    inline int get_index_dirty() {
       return index;
    }

    /**
     * update the index, that's it.
     */
    inline void set_index_dirty(int new_index) {
        index = new_index;
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* known_type() final;

};