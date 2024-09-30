// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"

class LinkedType : public TokenizedBaseType {
public:

    std::string type;
    ASTNode *linked;

    LinkedType(std::string type, CSTToken* token) : type(std::move(type)), TokenizedBaseType(token), linked(nullptr) {

    }

    [[deprecated]]
    LinkedType(std::string type, CSTToken* token, ASTNode* linked) : type(std::move(type)), TokenizedBaseType(token), linked(linked) {

    }

    LinkedType(std::string type, ASTNode* linked, CSTToken* token) : type(std::move(type)), TokenizedBaseType(token), linked(linked) {

    }

    uint64_t byte_size(bool is64Bit) override;

    BaseType* pure_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]]
    ValueType value_type() const override;

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    bool satisfies(ValueType value_type) override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Linked;
    }

//    bool satisfies(ASTAllocator& allocator, Value* value) override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<LinkedType *>(other)->linked == linked;
    }

    bool satisfies(BaseType *type) override;

    [[nodiscard]]
    LinkedType *copy(ASTAllocator& allocator) const override {
        auto t = new (allocator.allocate<LinkedType>()) LinkedType(type, token);
        t->linked = linked;
        return t;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

#endif

};