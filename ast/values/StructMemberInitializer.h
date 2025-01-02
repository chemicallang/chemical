// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * struct value initializer is an initializer that is inside a struct value
 * struct X { member : value } <--- member : value is the StructValueInitializer
 */
class StructMemberInitializer : public ASTNode {
public:

    /**
     * the name of the struct member being initialized
     */
    chem::string_view name;

    /**
     * the value used to initialize
     */
    Value* value;

    /**
     * the member we're linked with
     */
    ASTNode* member;

    /**
     * the reference to the struct value which holds this initializer
     */
    StructValue* struct_value;

    /**
     * constructor
     */
    StructMemberInitializer(
        chem::string_view name,
        Value* value,
        StructValue* struct_value,
        ASTNode* member = nullptr
    ) : name(name), value(value), struct_value(struct_value), member(member) {

    }

    ASTNodeKind kind() final {
        return ASTNodeKind::StructMemberInitializer;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    StructMemberInitializer* copy(ASTAllocator& allocator);

    SourceLocation encoded_location() final;

    ASTNode* parent() final;

};