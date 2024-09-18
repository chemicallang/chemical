// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public TokenizedBaseType {
public:

    static const PointerType void_ptr_instance;

    hybrid_ptr<BaseType> type;
    std::vector<std::unique_ptr<BaseType>> pures{};

    PointerType(std::unique_ptr<BaseType> type, CSTToken* token) : type(type.release(), true), TokenizedBaseType(token) {

    }

    PointerType(hybrid_ptr<BaseType> type, CSTToken* token) : type(std::move(type)), TokenizedBaseType(token) {

    }

    int16_t get_generic_iteration() override {
        return type->get_generic_iteration();
    }

    [[nodiscard]]
    std::unique_ptr<BaseType> create_child_type() const override {
        return std::unique_ptr<BaseType>(type->copy());
    }

    hybrid_ptr<BaseType> get_child_type() override {
        return hybrid_ptr<BaseType> { type.get(), false };
    }

    BaseType* known_child_type() override {
        return type.get();
    }

    BaseType* pure_type() override;

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType value_type) override {
        return type->satisfies(value_type);
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Pointer;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<PointerType *>(other)->type->is_same(type.get());
    }

    PointerType *pointer_type() override {
        return this;
    }

    [[nodiscard]]
    PointerType *copy() const override {
        return new PointerType(std::unique_ptr<BaseType>(type->copy()), token);
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};