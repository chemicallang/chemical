// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/values/UBigIntValue.h"
#include "ast/base/TypeLoc.h"

/**
 * will determine the size of a given type
 */
class SizeOfValue : public Value {
public:

    TypeLoc for_type;

    /**
     * constructor
     */
    constexpr SizeOfValue(
        TypeLoc for_type,
        SourceLocation location
    ) : Value(ValueKind::SizeOfValue, location), for_type(for_type) {

    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType();
    }

    Value* evaluated_value(InterpretScope &scope) override;

    SizeOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<SizeOfValue>()) SizeOfValue(for_type.copy(allocator), encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};