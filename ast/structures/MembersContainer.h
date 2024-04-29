// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include <optional>
#include <map>

class MembersContainer : public ASTNode {
public:

    void declare_and_link(SymbolResolver &linker) override;

    FunctionDeclaration *member(const std::string &name);

    ASTNode *child(const std::string &name) override;

    int child_index(const std::string &name) override;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    bool add_child_indexes(Codegen &gen, std::vector<llvm::Value *> &indexes, std::vector<std::unique_ptr<Value>> &u_inds) override;

#endif

    std::string representation() const;

    std::map<std::string, std::unique_ptr<StructMember>> variables; ///< The members of the struct.
    std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions;

};