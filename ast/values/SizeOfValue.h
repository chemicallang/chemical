// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/UBigIntValue.h"

/**
 * will determine the size of a given type
 */
class SizeOfValue : public Value {
public:

    BaseType* for_type;
    SourceLocation location;

    explicit SizeOfValue(
        BaseType *for_type,
        SourceLocation location
    ) : Value(ValueKind::SizeOfValue), for_type(for_type), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(location);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    Value* evaluated_value(InterpretScope &scope) override;

    SizeOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<SizeOfValue>()) SizeOfValue(for_type->copy(allocator), location);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};