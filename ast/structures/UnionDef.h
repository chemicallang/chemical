// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"

class UnionDef : public ExtendableMembersContainerNode {
public:

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    explicit UnionDef(std::string name);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    UnionDef *as_union_def() override {
        return this;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::StructType* get_struct_type(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
    ) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};