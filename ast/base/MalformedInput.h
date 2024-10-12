// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

/**
 * malformed input requires linking, we store in the ast anywhere a node, value or type
 * can be stored, when we link it, malformed input where correct input was desired
 * why store it ? because we link it to provide better diagnostics in IDE's
 * for example user writes if(x.y <-- yes he ends without writing any code after x.y
 * here user can write a dot and press ctrl + space and ask for completions, we must know
 * what x.y is before we do, they must be linked in this context, in the block where x.y is defined !
 */
class MalformedInput : public ASTNode, public BaseType, public Value {
public:

    std::vector<ASTAny*> any_things;
    ASTNode* parent_node;
    CSTToken* token;

    MalformedInput(ASTNode* parent, CSTToken* token) : parent_node(parent), token(token) {

    }

    ASTNodeKind kind() override {
        return ASTNodeKind::Malformed;
    }

    ValueKind val_kind() override {
        return ValueKind::Malformed;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<MalformedInput>()) MalformedInput(
            parent_node,
            token
        );
    }

    bool satisfies(BaseType *type) override {
        return false;
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) override {
        return false;
    }

    bool satisfies(ValueType type) override {
        return false;
    }

    bool is_same(BaseType *type) override {
        return false;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    CSTToken* cst_token() override {
        return token;
    }

    bool link(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final {
        link(linker);
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) final {
        link(linker);
        return false;
    }

};