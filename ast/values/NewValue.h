// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/PointerType.h"

class NewValue : public Value {
public:

    Value* value;
    PointerType ptr_type;

    /**
     * constructor
     */
    constexpr NewValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NewValue, &ptr_type, location), value(value), ptr_type(value->getType(), false) {

    }

    NewValue* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<NewValue>()) NewValue(
                value->copy(allocator),
                encoded_location()
        );
    }

    BaseType* create_type(ASTAllocator &allocator) override;

    BaseType* known_type() override;

    ASTNode* linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) override {
        return value->add_child_index(gen, indexes, name);
    }

#endif

};