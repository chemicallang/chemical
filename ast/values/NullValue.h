// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"
#include "ast/types/NullPtrType.h"

// representation is null
class NullValue : public Value {
public:

    inline NullValue(
        NullPtrType* nullType,
        SourceLocation location
    ) : Value(ValueKind::NullValue, nullType, location) {

    }

    inline NullPtrType* getType() const noexcept {
        return (NullPtrType*) Value::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    [[nodiscard]]
    std::string representation() const {
        return "null";
    }

    NullValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NullValue>()) NullValue(getType(), encoded_location());
    }

#ifdef COMPILER_BUILD

    static llvm::Value* null_llvm_value(Codegen &gen);

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<NullPtrType>()) NullPtrType();
    }

    BaseType* known_type() final {
        return (NullPtrType*) &NullPtrType::instance;
    }

};