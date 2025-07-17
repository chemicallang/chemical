// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class DereferenceValue : public Value {
private:

    Value* value;

public:

    /**
     * constructor
     */
    constexpr DereferenceValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::DereferenceValue, location), value(value) {

    }

    /**
     * constructor
     */
    constexpr DereferenceValue(
            Value* value,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::DereferenceValue, type, location), value(value) {

    }

    Value* getValue() noexcept {
        return value;
    }

    uint64_t byte_size(bool is64Bit) final {
        return value->byte_size(is64Bit);
    }

    bool determine_type(TypeBuilder& typeBuilder);

    DereferenceValue *copy(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    BaseType* create_type(ASTAllocator &allocator) final;

    Value* evaluated_value(InterpretScope &scope) override;

    ASTNode* linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_pointer(Codegen& gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) override {
        return value->add_child_index(gen, indexes, name);
    }

#endif

};