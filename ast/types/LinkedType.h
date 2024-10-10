// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"

class LinkedType : public TokenizedBaseType {
public:

    std::string type;
    ASTNode *linked;
    bool is_mutable;

    LinkedType(std::string type, CSTToken* token) : type(std::move(type)), TokenizedBaseType(token), linked(nullptr) {

    }

    [[deprecated]]
    LinkedType(std::string type, CSTToken* token, ASTNode* linked) : type(std::move(type)), TokenizedBaseType(token), linked(linked) {

    }

    LinkedType(std::string type, ASTNode* linked, CSTToken* token, bool is_mutable = false) : type(std::move(type)), TokenizedBaseType(token), linked(linked), is_mutable(is_mutable) {

    }

    uint64_t byte_size(bool is64Bit) override;

    BaseType* pure_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    ValueType value_type() const override;

    bool link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    bool satisfies(ValueType value_type) override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Linked;
    }

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<LinkedType *>(other)->linked == linked;
    }

    bool satisfies(BaseType *type) override;

    [[nodiscard]]
    LinkedType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<LinkedType>()) LinkedType(type, linked, token, is_mutable);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

#endif

};