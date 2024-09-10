// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ReferenceType : public TokenizedBaseType {
public:

    static const ReferenceType ref_ptr_instance;

    std::unique_ptr<BaseType> type;

    ReferenceType(std::unique_ptr<BaseType> type, CSTToken* token) : type(std::move(type)), TokenizedBaseType(token) {

    }

    int16_t get_generic_iteration() override {
        return type->get_generic_iteration();
    }

    [[nodiscard]]
    std::unique_ptr<BaseType> create_child_type() const override {
        return type->create_child_type();
    }

    hybrid_ptr<BaseType> get_child_type() override {
        return type->get_child_type();
    }

    BaseType* known_child_type() override {
        return type->known_child_type();
    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Reference;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    bool satisfies(ValueType value_type) override {
        return type->satisfies(value_type);
    }

    bool satisfies(Value *value) override {
        return type->satisfies(value);
    }

    bool satisfies(BaseType *given) override {
        return type->satisfies(given);
    }

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<ReferenceType *>(other)->type->is_same(type.get());
    }

    [[nodiscard]]
    ReferenceType *copy() const override {
        return new ReferenceType(type->copy_unique(), token);
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override {
        type->link(linker, type);
    }

    ASTNode *linked_node() override {
        return type->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};