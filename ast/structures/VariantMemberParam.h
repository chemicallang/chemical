// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMember;

class VariantMemberParam : public ASTNode {
public:

    chem::string_view name;
    BaseType* type;
    Value* def_value;
    VariantMember* parent_node;
    unsigned index;
    SourceLocation location;
    bool is_const;

    VariantMemberParam(
        chem::string_view name,
        unsigned index,
        bool is_const,
        BaseType* type,
        Value* def_value,
        VariantMember* parent_node,
        SourceLocation location
    ) : name(std::move(name)), index(index), type(type), def_value(def_value), parent_node(parent_node), location(location), is_const(is_const) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::VariantMemberParam;
    }

    VariantMemberParam* copy(ASTAllocator& allocator);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    uint64_t byte_size(bool is64Bit) final {
        return type->byte_size(is64Bit);
    }

    void accept(Visitor *visitor) final {
        throw std::runtime_error("VariantMemberParam::accept called");
    }

    BaseType* known_type() final {
        return type;
    }

    ASTNode* parent() final {
        return (ASTNode*) parent_node;
    }

    ASTNode* child(int index) final;

    int child_index(const chem::string_view &name) final;

    ASTNode* child(const chem::string_view &name) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};