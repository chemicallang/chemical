// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/IntType.h"

struct EnumMemberAttributes {
    bool deprecated;
};

class EnumMember : public ASTNode {
private:

    /**
     * this index means index inside the parent enum, it is not the value
     * for this enum member
     */
    unsigned int index;

public:

    chem::string_view name;
    /**
     * this init_value may be nullptr, if user didn't specify an explicit value for
     * this enum member
     */
    Value* init_value;
    EnumMemberAttributes attrs;

    /**
     * constructor
     */
    constexpr EnumMember(
        chem::string_view name,
        unsigned int index,
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

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) final;

    /**
     * the default index we'll use for this enum member, if user didn't specify an
     * explicit index for it
     */
    unsigned int get_default_index();

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* known_type() final;

};