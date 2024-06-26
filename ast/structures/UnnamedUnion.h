// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer {
public:

    explicit UnnamedUnion(
        std::string name
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNode *child(const std::string &name) override {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) override {
        return largest_member()->byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override {
        return true;
    }

#endif

};