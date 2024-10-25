// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public TokenizedBaseType {
public:

    static const PointerType void_ptr_instance;

    BaseType* type;
    bool is_mutable;
    std::vector<std::unique_ptr<BaseType>> pures{};

    PointerType(BaseType* type, CSTToken* token, bool is_mutable = false) : type(type), TokenizedBaseType(token), is_mutable(is_mutable) {

    }

    int16_t get_generic_iteration() final {
        return type->get_generic_iteration();
    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final {
        return type->copy(allocator);
    }

//    hybrid_ptr<BaseType> get_child_type() final {
//        return hybrid_ptr<BaseType> { type.get(), false };
//    }

    BaseType* known_child_type() final {
        return type;
    }

    BaseType* pure_type() final;

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType value_type) final {
        return type->satisfies(value_type);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Pointer;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Pointer;
    }

    bool satisfies(BaseType *type) final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && static_cast<PointerType *>(other)->type->is_same(type);
    }

    [[nodiscard]]
    PointerType *copy(ASTAllocator& allocator) const final {
        return new(allocator.allocate<PointerType>()) PointerType(type->copy(allocator), token, is_mutable);
    }

    bool link(SymbolResolver &linker) final;

    ASTNode *linked_node() final;

    /**
     * this type will be made mutable, if the child type is mutable, for example
     * this type is *mut Thing <-- pointer is not mutable but it's child type
     * when this method is called, we will set pointer to mutable too
     */
    void make_mutable_on_child() {
        is_mutable = type->is_mutable(type->kind());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};