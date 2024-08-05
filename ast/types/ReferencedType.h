// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"

class ReferencedType : public BaseType {
public:

    std::string type;

    ASTNode *linked;

    ReferencedType(std::string type) : type(std::move(type)) {}

    ReferencedType(std::string type, ASTNode* linked) : type(std::move(type)), linked(linked) {}

    uint64_t byte_size(bool is64Bit) override;

    hybrid_ptr<BaseType> get_pure_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ValueType value_type() const override;

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    ASTNode *linked_node() override;

    bool satisfies(ValueType value_type) override;

    BaseTypeKind kind() const override {
        return BaseTypeKind::Referenced;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<ReferencedType *>(other)->linked == linked;
    }

    virtual BaseType *copy() const {
        auto t = new ReferencedType(type);
        t->linked = linked;
        return t;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

#endif

};