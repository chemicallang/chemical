// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"

class CastedValue : public Value {
public:

    Value* value;
    SourceLocation type_location;

    /**
     * constructor
     */
    constexpr CastedValue(
        Value* value,
        TypeLoc type,
        SourceLocation location
    ) : Value(ValueKind::CastedValue, const_cast<BaseType*>(type.getType()), location), value(value), type_location(type.getLocation()) {

    }


    CastedValue* copy(ASTAllocator& allocator) final {
        return new CastedValue(
                value->copy(allocator),
                { getType()->copy(allocator), type_location },
                encoded_location()
        );
    }

    Value* evaluated_value(InterpretScope &scope) final;

    BaseType* known_type() final {
        return getType();
    }

    ASTNode *linked_node() final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};