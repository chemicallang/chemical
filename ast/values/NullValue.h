// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// representation is null
class NullValue : public Value {
public:

    PointerType* expected = nullptr;
    SourceLocation location;

    explicit NullValue(SourceLocation location) : location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    ValueKind val_kind() final {
        return ValueKind::NullValue;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    std::string representation() const {
        return "null";
    }

    NullValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NullValue>()) NullValue(location);
    }

#ifdef COMPILER_BUILD

    static llvm::Value* null_llvm_value(Codegen &gen);

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Pointer;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Pointer;
    }

    BaseType* create_type(ASTAllocator &allocator) final;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

};