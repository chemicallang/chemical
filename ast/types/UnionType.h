// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ordered_map.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/VariablesContainer.h"

class UnionType : public BaseType, public VariablesContainer {
public:

    chem::string_view name;
    SourceLocation location;

    UnionType(chem::string_view name, SourceLocation location) : name(name), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    void accept(Visitor *visitor) {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Union;
    }

    ValueType value_type() const {
        return ValueType::Union;
    }

    uint64_t byte_size(bool is64Bit) {
        return largest_member_byte_size(is64Bit);
    }

    bool equals(UnionType *type) const {
        return type->byte_size(true) == const_cast<UnionType*>(this)->byte_size(true);
    }

    bool is_same(BaseType *type) final {
        return kind() == type->kind() && equals(static_cast<UnionType *>(type));
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<UnionType>()) UnionType(name, location);
    }

    bool satisfies(ValueType type) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index);

#endif

};