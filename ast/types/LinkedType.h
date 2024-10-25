// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"

class LinkedType : public TokenizedBaseType {
public:

    std::string type;
    ASTNode *linked;

    LinkedType(std::string type, SourceLocation location) : type(std::move(type)), TokenizedBaseType(location), linked(nullptr) {

    }

    [[deprecated]]
    LinkedType(std::string type, SourceLocation location, ASTNode* linked) : type(std::move(type)), TokenizedBaseType(location), linked(linked) {

    }

    LinkedType(std::string type, ASTNode* linked, SourceLocation location) : type(std::move(type)), TokenizedBaseType(location), linked(linked) {

    }

    uint64_t byte_size(bool is64Bit) final;

    BaseType* pure_type() final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    ValueType value_type() const final;

    bool link(SymbolResolver &linker);

    ASTNode *linked_node() final;

    bool satisfies(ValueType value_type) final;

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Linked;
    }

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && static_cast<LinkedType *>(other)->linked == linked;
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    LinkedType *copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<LinkedType>()) LinkedType(type, linked, location);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

#endif

};