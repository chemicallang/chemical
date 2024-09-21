// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/UBigIntValue.h"

/**
 * will determine the size of a given type
 */
class SizeOfValue : public UBigIntValue {
public:

    BaseType* for_type;

    explicit SizeOfValue(BaseType *for_type, CSTToken* token);

    ValueKind val_kind() override {
        return ValueKind::SizeOfValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    void calculate_size(bool is64Bit);

    SizeOfValue* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<SizeOfValue>()) SizeOfValue(for_type->copy(allocator), token);
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

};