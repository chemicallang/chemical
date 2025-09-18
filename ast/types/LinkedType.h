// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"
#include "ast/base/ASTNode.h"
#include "ast/base/LocatedIdentifier.h"
#include "std/chem_string_view.h"

struct LinkedTypeAttrs {

    bool is_named = false;

    bool is_value = false;

};

class LinkedType : public BaseType {
public:

    ASTNode *linked;
    LinkedTypeAttrs attrs;

    constexpr LinkedType(ASTNode* linked) : BaseType(BaseTypeKind::Linked), linked(linked) {

    }

    constexpr LinkedType(
            ASTNode* linked,
            bool is_named,
            bool is_value
    ) : BaseType(BaseTypeKind::Linked), linked(linked), attrs(is_named, is_value) {

    }

    inline bool is_value() {
        return attrs.is_value;
    }

    inline bool is_named() {
        return attrs.is_named;
    }

    uint64_t byte_size(bool is64Bit) final;

    ASTNode* linked_node() final {
        return linked;
    }

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && static_cast<LinkedType *>(other)->linked == linked;
    }

    bool satisfies(BaseType *type) final;

    bool satisfies(Value *value, bool assignment) override;

    [[nodiscard]]
    LinkedType *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<LinkedType>()) LinkedType(linked);
    }

    inline chem::string_view linked_name() {
        const auto id = linked->get_located_id();
        if(id) {
            return id->identifier;
        } else {
            return "";
        }
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

#endif

};

class NamedLinkedType : public LinkedType {
private:

    chem::string_view link_name;

public:

    constexpr NamedLinkedType(
        chem::string_view type
    ) : LinkedType(nullptr, true, false), link_name(type) {

    }

    constexpr NamedLinkedType(
        chem::string_view type,
        ASTNode* linked
    ) : LinkedType(linked, true, false), link_name(type) {

    }

    /**
     * avoid using it, however this could provide debugging capabilities if you know linked type is a named linked type
     */
    inline chem::string_view debug_link_name() {
        return link_name;
    }

};