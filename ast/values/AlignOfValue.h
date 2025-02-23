// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/UBigIntValue.h"

/**
 * will determine the size of a given type
 */
class AlignOfValue : public Value {
public:

    BaseType* for_type;

    explicit AlignOfValue(
        BaseType *for_type,
        SourceLocation location
    ) : Value(ValueKind::AlignOfValue, location), for_type(for_type) {

    }


    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(encoded_location());
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    AlignOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<AlignOfValue>()) AlignOfValue(for_type->copy(allocator), encoded_location());
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};