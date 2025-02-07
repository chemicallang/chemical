// Copyright (c) Qinetik 2024.

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
    SourceLocation location;

    explicit NotValue(Value* value, SourceLocation location) : value(value), location(location) {}

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::NotValue;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    bool primitive() final;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    NotValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NotValue>()) NotValue(value->copy(allocator), location);
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