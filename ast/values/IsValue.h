// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class IsValue : public Value {
public:

    Value* value;
    BaseType* type;
    bool is_negating;

    /**
     * constructor
     */
    constexpr IsValue(
            Value* value,
            BaseType* type,
            bool is_negating,
            SourceLocation location
    ) : Value(ValueKind::IsValue, location), value(value), type(type), is_negating(is_negating) {

    }


    IsValue *copy(ASTAllocator& allocator) final;

    /**
     * std::nullopt means unknown, true or false means it evaluated
     */
    std::optional<bool> get_comp_time_result();

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &BoolType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<BoolType>()) BoolType(encoded_location());
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Bool;
    }

};