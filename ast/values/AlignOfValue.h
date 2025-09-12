// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/values/IntNumValue.h"

/**
 * will determine the size of a given type
 */
class AlignOfValue : public Value {
public:

    TypeLoc for_type;

    /**
     * constructor
     */
    constexpr AlignOfValue(
            TypeLoc for_type,
            U64Type* type,
            SourceLocation location
    ) : Value(ValueKind::AlignOfValue, type, location), for_type(for_type) {

    }

    U64Type* getType() {
        return (U64Type*) Value::getType();
    }

    AlignOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<AlignOfValue>()) AlignOfValue(
                for_type.copy(allocator),
                getType(),
                encoded_location()
        );
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};