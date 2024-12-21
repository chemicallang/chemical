// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class DereferenceValue : public Value {
public:

    Value* value;
    SourceLocation location;

    explicit DereferenceValue(Value* value, SourceLocation location);

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::DereferenceValue;
    }

    uint64_t byte_size(bool is64Bit) final {
        return value->byte_size(is64Bit);
    }

    DereferenceValue *copy(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

    BaseType* create_type(ASTAllocator &allocator) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ASTNode* linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_pointer(Codegen& gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

};