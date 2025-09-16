// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public BaseType {
protected:

    PointerType(const PointerType& other) : BaseType(BaseTypeKind::Pointer), type(other.type), is_mutable(other.is_mutable) {

    }

public:

    friend class NewValue;

    static const PointerType void_ptr_instance;

    BaseType* type;
    bool is_mutable;

    /**
     * constructor
     */
    constexpr PointerType(BaseType* type, bool is_mutable) : type(type), BaseType(BaseTypeKind::Pointer), is_mutable(is_mutable) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final {
        return type->copy(allocator);
    }

    BaseType* known_child_type() final {
        return type;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    bool satisfies(BaseType *type) final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && static_cast<PointerType *>(other)->type->is_same(type);
    }

    [[nodiscard]]
    PointerType *copy(ASTAllocator& allocator) final {
        return new(allocator.allocate<PointerType>()) PointerType(type->copy(allocator), is_mutable);
    }

    ASTNode *linked_node() final;

    /**
     * this type will be made mutable, if the child type is mutable, for example
     * this type is *mut Thing <-- pointer is not mutable but it's child type
     * when this method is called, we will set pointer to mutable too
     */
    void make_mutable_on_child() {
        is_mutable = type->is_mutable();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

#endif

};