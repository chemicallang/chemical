// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/VariablesContainer.h"

class StructType : public BaseType, public VariablesContainer {
public:

    chem::string_view name;
    SourceLocation location;

    StructType(chem::string_view name, SourceLocation location) : name(name), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    void accept(Visitor *visitor) {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Struct;
    }

    [[nodiscard]]
    ValueType value_type() const {
        return ValueType::Struct;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<StructType>()) StructType(name, location);
    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) final {
        return kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Struct;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

    llvm::Type *llvm_param_type(Codegen &gen);

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index);

#endif

};