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
        UBigIntType* type,
        SourceLocation location
    ) : Value(ValueKind::SizeOfValue, type, location), for_type(for_type) {

    }

    inline UBigIntType* getType() const noexcept {
        return (UBigIntType*) Value::getType();
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType();
    }

    Value* evaluated_value(InterpretScope &scope) override;

    SizeOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<SizeOfValue>()) SizeOfValue(
                for_type.copy(allocator), getType(), encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};