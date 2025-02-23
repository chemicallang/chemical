// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ReferenceType : public BaseType {
public:

    BaseType* type;
    bool is_mutable;

    ReferenceType(BaseType* type, SourceLocation location, bool is_mutable = false) : type(type), BaseType(BaseTypeKind::Reference, location), is_mutable(is_mutable) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final {
        return type->copy(allocator);
    }

//    hybrid_ptr<BaseType> get_child_type() final {
//        return type->get_child_type();
//    }

    BaseType* known_child_type() final {
        return type->known_child_type();
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(BaseType* given, Value* value, bool assignment);

    bool satisfies(BaseType *given) final {
        return satisfies(given, nullptr, false);
    }

    bool satisfies(ASTAllocator& allocator, Value* value, bool assignment) final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && other->as_reference_type_unsafe()->type->is_same(type);
    }

    [[nodiscard]]
    ReferenceType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ReferenceType>()) ReferenceType(type->copy(allocator), encoded_location(), is_mutable);
    }

    bool link(SymbolResolver &linker) final {
        return type->link(linker);
    }

    ASTNode *linked_node() final {
        return type->linked_node();
    }

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

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};