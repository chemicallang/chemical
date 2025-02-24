// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a not operator !value
class NotValue : public Value {
public:

    Value* value;

    explicit NotValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NotValue, location), value(value) {}

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    bool primitive() final;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    NotValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NotValue>()) NotValue(value->copy(allocator), encoded_location());
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return value->type_kind();
    }

    BaseType* create_type(ASTAllocator &allocator) final;

    Value* evaluated_value(InterpretScope &scope) override;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

};