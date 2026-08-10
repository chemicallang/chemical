// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"
#include "ast/base/TypeLoc.h"

/**
 * will determine the byte offset of a member within a given type
 */
class OffsetOfValue : public Value {
public:

    TypeLoc for_type;

    /**
     * the name of the member whose offset is being queried
     * (a view into the AST arena)
     */
    chem::string_view member_name;

    /**
     * constructor
     */
    constexpr OffsetOfValue(
        TypeLoc for_type,
        chem::string_view member_name,
        U64Type* type,
        SourceLocation location
    ) : Value(ValueKind::OffsetOfValue, type, location), for_type(for_type), member_name(member_name) {

    }

    inline U64Type* getType() const noexcept {
        return (U64Type*) Value::getType();
    }

    Value* evaluated_value(InterpretScope &scope) override;

    OffsetOfValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<OffsetOfValue>()) OffsetOfValue(
                for_type.copy(allocator), member_name, getType(), encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};
