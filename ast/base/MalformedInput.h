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
    SourceLocation location;

    MalformedInput(ASTNode* parent, SourceLocation location) : parent_node(parent), location(location) {

    }

    ASTNodeKind kind() final {
        return ASTNodeKind::Malformed;
    }

    ValueKind val_kind() final {
        return ValueKind::Malformed;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* copy(ASTAllocator &allocator) const final {
        return new (allocator.allocate<MalformedInput>()) MalformedInput(
            parent_node,
            location
        );
    }

    bool satisfies(BaseType *type) final {
        return false;
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final {
        return false;
    }

    bool satisfies(ValueType type) final {
        return false;
    }

    bool is_same(BaseType *type) final {
        return false;
    }

    ASTNode* parent() final {
        return parent_node;
    }

    SourceLocation encoded_location() final {
        return location;
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