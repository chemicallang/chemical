// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class CastedValue : public Value {
public:

    Value* value;
    BaseType* type;
    CSTToken* token;

    CastedValue(Value* value, BaseType* type, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::CastedValue;
    }

    CastedValue *copy(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { type.get(), false };
//    }

    BaseType* known_type() override {
        return type;
    }

    BaseType* create_type(ASTAllocator &allocator) override {
        return type->copy(allocator);
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return type->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return type->kind();
    }

};