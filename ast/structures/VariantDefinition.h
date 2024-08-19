// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/ReferencedType.h"

class VariantDefinition : public ExtendableMembersContainerNode {
public:

    AccessSpecifier specifier = AccessSpecifier::Internal;
    ASTNode* parent_node;
    ReferencedType ref_type;

#ifdef COMPILER_BUILD
    /**
     * the key here is the generic iteration, where as the value corresponds to
     * a llvm struct type, we create a struct type once, and then cache it
     */
    std::unordered_map<int16_t, llvm::StructType*> llvm_struct_types;
#endif

    /**
     * constructor
     */
    VariantDefinition(
        std::string name,
        ASTNode* parent_node
    );

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    bool is_generic() {
        return !generic_params.empty();
    }

    std::string ns_node_identifier() override {
        return name;
    }

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    BaseType* known_type() override;

    [[nodiscard]]
    ValueType value_type() const override;

    ASTNode* child(const std::string &child_name) override;

    uint64_t byte_size(bool is64Bit) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen, int16_t iteration);

    llvm::Type* llvm_param_type(Codegen &gen) override;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    void code_gen(Codegen &gen) override;

    void code_gen_generic(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

};