// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <memory>
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

class CoreNodes;
class ImplementationsIndex;

// A value that's preceded by a bitwise not operator ~value
class BitwiseNot : public Value {
private:

    Value* value;

public:

    constexpr BitwiseNot(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::BitwiseNot, value->getType(), location), value(value) {}

    constexpr BitwiseNot(
            Value* value,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::BitwiseNot, type, location), value(value) {}

    Value* getValue() const noexcept {
        return value;
    }

    bool primitive() final;

    bool compile_time_computable() override {
        return value->compile_time_computable();
    }


    BitwiseNot* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BitwiseNot>()) BitwiseNot(value->copy(allocator), getType(), encoded_location());
    }

    Value* evaluated_value(InterpretScope &scope) override;

    void determine_type(ASTDiagnoser& diagnoser, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex);

};
