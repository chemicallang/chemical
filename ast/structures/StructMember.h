// Copyright (c) Qinetik 2024.

#pragma once

#include <map>
#include "ast/base/Value.h"
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
    ASTNode* parent_node;
    SourceLocation location;
    StructMemberAttributes attrs;

    StructMember(
            std::string name,
            BaseType* type,
            Value* defValue,
            ASTNode* parent_node,
            SourceLocation location,
            bool is_const = false,
            AccessSpecifier specifier = AccessSpecifier::Public
    );

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

    ASTNodeKind kind() final {
        return ASTNodeKind::StructMember;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    Value *default_value() final {
        if(defValue) return defValue;
        return nullptr;
    }

    BaseDefMember *copy_member(ASTAllocator& allocator) final;

    bool get_is_const() final {
        return is_const();
    }

    void accept(Visitor *visitor) final;

    void declare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *child(const std::string &name) final;

    Value *holding_value() final {
        return defValue ? defValue : nullptr;
    }

    BaseType *known_type() final {
        return type;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::FunctionType* llvm_func_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};