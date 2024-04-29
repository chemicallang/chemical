// Copyright (c) Qinetik 2024.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"

class StructMember : public ASTNode {
public:

    std::string name;

    std::unique_ptr<BaseType> type;

    std::optional<std::unique_ptr<Value>> defValue;

    StructMember(std::string name, std::unique_ptr<BaseType> type, std::optional<std::unique_ptr<Value>> defValue);

    void accept(Visitor &visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    bool add_child_indexes(Codegen &gen, std::vector<llvm::Value *> &indexes, std::vector<std::unique_ptr<Value>> &u_inds) override;

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    std::string representation() const override;

};