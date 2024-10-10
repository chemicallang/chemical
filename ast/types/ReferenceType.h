// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ReferenceType : public TokenizedBaseType {
public:

    BaseType* type;
    bool is_mutable = false;

    ReferenceType(BaseType* type, CSTToken* token) : type(type), TokenizedBaseType(token) {

    }

    int16_t get_generic_iteration() override {
        return type->get_generic_iteration();
    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const override {
        return type->create_child_type(allocator);
    }

//    hybrid_ptr<BaseType> get_child_type() override {
//        return type->get_child_type();
//    }

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

    bool satisfies(ASTAllocator& allocator, Value* value) override {
        return type->satisfies(allocator, value);
    }

    bool satisfies(BaseType *given) override {
        return type->satisfies(given);
    }

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<ReferenceType *>(other)->type->is_same(type);
    }

    [[nodiscard]]
    ReferenceType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<ReferenceType>()) ReferenceType(type->copy(allocator), token);
    }

    bool link(SymbolResolver &linker) override {
        return type->link(linker);
    }

    ASTNode *linked_node() override {
        return type->linked_node();
    }

    /**
     * this type will be made mutable, if the child type is mutable, for example
     * this type is *mut Thing <-- pointer is not mutable but it's child type
     * when this method is called, we will set pointer to mutable too
     */
    void make_mutable_on_child() {
        is_mutable = type->is_mutable(type->kind());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};