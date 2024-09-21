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
    CSTToken* token;

    explicit DereferenceValue(Value* value, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::DereferenceValue;
    }

    uint64_t byte_size(bool is64Bit) override {
        return value->byte_size(is64Bit);
    }

    DereferenceValue *copy(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    BaseType* create_type(ASTAllocator &allocator) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen& gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

};