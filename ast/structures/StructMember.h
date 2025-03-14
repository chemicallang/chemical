// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "BaseDefMember.h"

struct StructMemberAttributes {

    AccessSpecifier specifier;

    bool is_const;

    bool deprecated;

};

class StructMember : public BaseDefMember {
public:

    BaseType* type;
    Value* defValue;
    StructMemberAttributes attrs;

    /**
     * constructor
     */
    constexpr StructMember(
            chem::string_view name,
            BaseType* type,
            Value* defValue,
            ASTNode* parent_node,
            SourceLocation location,
            bool is_const = false,
            AccessSpecifier specifier = AccessSpecifier::Public
    ) : BaseDefMember(name, ASTNodeKind::StructMember, parent_node, location), type(type), defValue(defValue),
        attrs(specifier, is_const, false)
    {

    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_const() {
        return attrs.is_const;
    }

    inline void set_const(bool value) {
        attrs.is_const = value;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    Value *default_value() final {
        if(defValue) return defValue;
        return nullptr;
    }

    BaseDefMember *copy_member(ASTAllocator& allocator) final {
        Value* def_value = defValue ? defValue->copy(allocator) : nullptr;
        const auto new_mem = new (allocator.allocate<StructMember>()) StructMember(name, type->copy(allocator), def_value, parent(), encoded_location());
        new_mem->attrs = attrs;
        return new_mem;
    }

    bool get_is_const() final {
        return is_const();
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void link_signature(SymbolResolver &linker) override;

    ASTNode *child(const chem::string_view &name) final;

    Value *holding_value() final {
        return defValue ? defValue : nullptr;
    }

    BaseType *known_type() final {
        return type;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};