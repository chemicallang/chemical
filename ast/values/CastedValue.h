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

    CSTToken* cst_token() final {
        return token;
    }

    ValueKind val_kind() final {
        return ValueKind::CastedValue;
    }

    CastedValue *copy(ASTAllocator& allocator) final;

    Value* evaluated_value(InterpretScope &scope) final {
        return value->evaluated_value(scope);
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { type.get(), false };
//    }

    BaseType* known_type() final {
        return type;
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return type->copy(allocator);
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    ASTNode *linked_node() final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

#endif

    [[nodiscard]]
    ValueType value_type() const final {
        return type->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return type->kind();
    }

};