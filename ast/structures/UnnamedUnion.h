// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer {
public:

    ASTNode* parent_node;
    AccessSpecifier specifier;

    UnnamedUnion(
        chem::string_view name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : BaseDefMember(name, ASTNodeKind::UnnamedUnion, location), parent_node(parent_node), specifier(specifier) {

    }


//    VariablesContainer *variables_container() final {
//        return this;
//    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed struct's const
        return false;
    }

    BaseDefMember* copy_member(ASTAllocator &allocator) final;

    VariablesContainer* copy_container(ASTAllocator &allocator) final;

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    ASTNode *child(const chem::string_view &name) final {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) final {
        return largest_member()->byte_size(is64Bit);
    }

//    ASTNode *linked_node() final {
//        return this;
//    }

//    [[nodiscard]]
//    BaseType *copy(ASTAllocator& allocator) const final;

//    BaseType* known_type() final {
//        return this;
//    }

    BaseType* create_value_type(ASTAllocator &allocator) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Union;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const chem::string_view &name
    ) final {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};