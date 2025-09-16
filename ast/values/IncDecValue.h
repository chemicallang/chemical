// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

class IncDecValue : public Value {
private:

    Value* value;

public:

    bool increment;
    bool post;

    /**
     * constructor
     */
    constexpr IncDecValue(
        Value* value,
        bool increment,
        bool post,
        SourceLocation location
    ) : Value(ValueKind::IncDecValue, value->getType(), location), value(value), increment(increment), post(post) {

    }

    inline Value* getValue() const noexcept {
        return value;
    }

    inline void setValue(Value* newValue) noexcept {
        value = newValue;
        setType(newValue->getType());
    }

    chem::string_view get_overloaded_func_name();

    BaseType* determine_type(ASTDiagnoser& diagnoser);

    Value* evaluated_value(InterpretScope &scope) override;

    IncDecValue* copy(ASTAllocator &allocator) final {
        return new (allocator.allocate<IncDecValue>()) IncDecValue(
            value->copy(allocator),
            increment,
            post,
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override {
        return value->llvm_type(gen);
    }

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif


};