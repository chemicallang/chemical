// Copyright (c) Qinetik 2024.

#pragma once

#include "ordered_map.h"
#include "BaseDefMember.h"
#include <string>
#include <memory>

class VariablesContainer {
public:

    tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> variables;

    int variable_index(const std::string &name);

    uint64_t total_byte_size(bool is64Bit);

    BaseDefMember *child_def_member(const std::string &name);

    BaseDefMember* largest_member();

    virtual void declare_and_link(SymbolResolver &linker);

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> elements_type(Codegen &gen);

    bool llvm_struct_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    );

    bool llvm_union_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    );

#endif

};