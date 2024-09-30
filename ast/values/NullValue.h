// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// representation is null
class NullValue : public Value {
public:

    PointerType* expected = nullptr;
    CSTToken* token;

    explicit NullValue(CSTToken* token) : token(token) {

    }

    CSTToken* cst_token() override {
        return token;
    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    ValueKind val_kind() override {
        return ValueKind::NullValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    std::string representation() const {
        return "null";
    }

    NullValue* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<NullValue>()) NullValue(token);
    }

#ifdef COMPILER_BUILD

    static llvm::Value* null_llvm_value(Codegen &gen);

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    BaseType* create_type(ASTAllocator &allocator) override;

//    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

};