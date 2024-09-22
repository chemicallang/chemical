// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ordered_map.h"
#include "ast/structures/BaseDefMember.h"

class VariablesContainer;

class UnionType : public BaseType {
public:

    UnionType() = default;

    virtual VariablesContainer* variables_container() = 0;

    virtual std::string union_name() {
        return "";
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Union;
    }

    ValueType value_type() const override {
        return ValueType::Union;
    }

    uint64_t byte_size(bool is64Bit) override;

    bool equals(UnionType *type) const {
        return type->byte_size(true) == const_cast<UnionType*>(this)->byte_size(true);
    }

    bool is_same(BaseType *type) override {
        return kind() == type->kind() && equals(static_cast<UnionType *>(type));
    }

    [[nodiscard]]
    BaseType *copy(ASTAllocator& allocator) const override = 0;

    bool satisfies(ValueType type) override;

    virtual bool is_anonymous() {
        return true;
    }

#ifdef COMPILER_BUILD

    virtual llvm::StructType *llvm_union_get_stored_type() {
        return nullptr;
    }

    virtual void llvm_union_type_store(llvm::StructType* type) {
        // does nothing
    }

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

#endif

};