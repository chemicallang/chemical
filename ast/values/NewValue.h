// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/PointerType.h"

class NewValue : public Value {
public:

    Value* value;
    SourceLocation location;
    // TODO remove this
    PointerType ptr_type;

    NewValue(Value* value, SourceLocation location) : value(value), location(location), ptr_type(nullptr, 0, false) {

    }

    ValueKind val_kind() override {
        return ValueKind::NewValue;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    BaseType* create_type(ASTAllocator &allocator) override;

    BaseType* known_type() override;

    ASTNode* linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override {
        return value->add_child_index(gen, indexes, name);
    }

#endif

};