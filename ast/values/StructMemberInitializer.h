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
    std::string name;

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
        std::string name,
        Value* value,
        StructValue* struct_value,
        ASTNode* member = nullptr
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::StructMemberInitializer;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    StructMemberInitializer* copy(ASTAllocator& allocator);

    CSTToken *cst_token() override;

    ASTNode* parent() override;

};