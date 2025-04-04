// Copyright (c) Chemical Language Foundation 2025.

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

    /**
     * constructor
     */
    constexpr CastedValue(
        Value* value,
        BaseType* type,
        SourceLocation location
    ) : Value(ValueKind::CastedValue, location), value(value), type(type) {

    }


    CastedValue *copy(ASTAllocator& allocator) final;

    Value* evaluated_value(InterpretScope &scope) final;

    BaseType* known_type() final {
        return type;
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return type->copy(allocator);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    ASTNode *linked_node() final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};