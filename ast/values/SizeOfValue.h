// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/UBigIntValue.h"

/**
 * will determine the size of a given type
 */
class SizeOfValue : public UBigIntValue {
public:

    std::unique_ptr<BaseType> for_type;

    explicit SizeOfValue(BaseType *for_type);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    void calculate_size(bool is64Bit);

    SizeOfValue* copy() override {
        return new SizeOfValue(for_type->copy());
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

};