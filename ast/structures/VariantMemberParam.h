// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class VariantMember;

class VariantMemberParam : public ASTNode {
public:

    std::string name;
    BaseType* type;
    Value* def_value;
    VariantMember* parent_node;
    unsigned index;
    SourceLocation location;
    bool is_const;

    VariantMemberParam(
        std::string name,
        unsigned index,
        bool is_const,
        BaseType* type,
        Value* def_value,
        VariantMember* parent_node,
        SourceLocation location
    );

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::VariantMemberParam;
    }

    VariantMemberParam* copy(ASTAllocator& allocator);

    void declare_and_link(SymbolResolver &linker) final;

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

    int child_index(const std::string &name) final;

    ASTNode* child(const std::string &name) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

#endif

};