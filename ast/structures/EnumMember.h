// Copyright (c) Qinetik 2024.

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
    EnumDeclaration* parent_node;
    /**
     * this init_value may be nullptr, if user didn't specify an explicit value for
     * this enum member
     */
    Value* init_value;
    SourceLocation location;
    EnumMemberAttributes attrs;

    EnumMember(
        chem::string_view name,
        unsigned int index,
        Value* init_value,
        EnumDeclaration* parent_node,
        SourceLocation location
    ) : name(name), index(index), init_value(init_value), parent_node(parent_node), location(location),
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
        // TODO should enum members be visited or not ?
    }

    void declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) final;

    /**
     * the default index we'll use for this enum member, if user didn't specify an
     * explicit index for it
     */
    unsigned int get_default_index();

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    BaseType* known_type() final;

};