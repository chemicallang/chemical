// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/UBigIntValue.h"

/**
 * will determine the size of a given type
 */
class SizeOfValue : public Value {
public:

    BaseType* for_type;
    CSTToken* token;

    explicit SizeOfValue(BaseType *for_type, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::SizeOfValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(nullptr);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    SizeOfValue* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<SizeOfValue>()) SizeOfValue(for_type->copy(allocator), token);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

};