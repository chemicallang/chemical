// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/IntType.h"

struct EnumMemberAttributes {
    bool deprecated;
};

class EnumMember : public ASTNode {
public:

    std::string name;
    unsigned int index;
    EnumDeclaration* parent_node;
    Value* init_value;
    SourceLocation location;
    EnumMemberAttributes attrs;

    EnumMember(
        std::string  name,
        unsigned int index,
        Value* init_value,
        EnumDeclaration* parent_node,
        SourceLocation location
    ) : name(std::move(name)), index(index), init_value(init_value), parent_node(parent_node), location(location),
       attrs(false)
    {

    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::EnumMember;
    }

    ASTNode *parent() final {
        return (ASTNode*) parent_node;
    }

    void accept(Visitor *visitor) final {

    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* known_type() final;

};