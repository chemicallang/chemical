// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ordered_map.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/VariablesContainer.h"

class UnionTypeCopy : public BaseType {
    using BaseType::BaseType;
    BaseType * copy(ASTAllocator &allocator) override;
};

class UnionType : public UnionTypeCopy, public ASTNode, public VariablesContainer {
public:

    chem::string_view name;

    /**
     * constructor
     */
    UnionType(
        chem::string_view name,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnionType, parent, location), UnionTypeCopy(BaseTypeKind::Union), name(name) {

    }

    uint64_t byte_size(bool is64Bit) {
        return largest_member_byte_size(is64Bit);
    }

    bool equals(UnionType *type) const {
        return type->byte_size(true) == const_cast<UnionType*>(this)->byte_size(true);
    }

    bool is_same(BaseType *type) final {
        return BaseType::kind() == type->kind() && equals(static_cast<UnionType *>(type));
    }

    VariablesContainer * as_variables_container() override {
        return this;
    }

    ASTNode* linked_node() override {
        return this;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index);

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};