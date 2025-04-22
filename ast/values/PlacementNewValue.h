// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/PointerType.h"

class PlacementNewValue : public Value {
public:

    Value* pointer;
    Value* value;
    // TODO remove this
    PointerType ptr_type;

    /**
     * constructor
     */
    inline constexpr PlacementNewValue(
        Value* pointer,
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::PlacementNewValue, location), pointer(pointer), value(value), ptr_type(nullptr, false) {

    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    PlacementNewValue* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<PlacementNewValue>()) PlacementNewValue(
            pointer->copy(allocator), value->copy(allocator), encoded_location()
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