// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include <optional>
#include <map>
#include "StructMember.h"
#include "ordered_map.h"
#include "ast/base/AnnotableNode.h"

class MembersContainer : public AnnotableNode {
public:

    void declare_and_link(SymbolResolver &linker) override;

    FunctionDeclaration *member(const std::string &name);

    ASTNode *child(const std::string &name) override;

    int child_index(const std::string &name) override;

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::string representation() const;

    tsl::ordered_map<std::string, std::unique_ptr<StructMember>> variables; ///< The members of the struct.
    tsl::ordered_map<std::string, std::unique_ptr<FunctionDeclaration>> functions;

};