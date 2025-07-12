// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/values/UBigIntValue.h"

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
        SourceLocation location
    ) : Value(ValueKind::AlignOfValue, location), for_type(for_type) {

    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType();
    }

    AlignOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<AlignOfValue>()) AlignOfValue(for_type.copy(allocator), encoded_location());
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};