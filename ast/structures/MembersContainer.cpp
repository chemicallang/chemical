// Copyright (c) Qinetik 2024.

#include "MembersContainer.h"

#include <ranges>
#include "compiler/SymbolResolver.h"
#include "StructMember.h"
#include "FunctionDeclaration.h"
#include "VariablesContainer.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

bool VariablesContainer::llvm_struct_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &child_name
) {
    auto index = variable_index(child_name);
    if (index == -1) {
        return false;
    }
    if (indexes.empty()) {
        indexes.emplace_back(gen.builder->getInt32(0));
    }
    indexes.emplace_back(gen.builder->getInt32(index));
    return true;
}

bool VariablesContainer::llvm_union_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
) {
    auto largest = largest_member();
    if(largest) {
        auto value_type = largest->get_value_type();
        // this should only be added if we are not inlining struct types inside union
        if(value_type->value_type() == ValueType::Struct) {
            if(indexes.empty()) {
                indexes.emplace_back(gen.builder->getInt32(0));
            }
            indexes.emplace_back(gen.builder->getInt32(0));
        }
    } else {
        return true;
    }
    return true;
}

std::vector<llvm::Type *> VariablesContainer::elements_type(Codegen &gen) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size());
    for (const auto &var: variables) {
        vec.push_back(var.second->llvm_type(gen));
    }
    return vec;
}

#endif

BaseDefMember *VariablesContainer::child_def_member(const std::string &name) {
    auto found = variables.find(name);
    if (found != variables.end()) {
        return found->second.get();
    } else {
        return nullptr;
    }
}

BaseDefMember* VariablesContainer::largest_member() {
    BaseDefMember* member = nullptr;
    for(auto& var : variables) {
        if(member == nullptr || var.second->byte_size(true) > member->byte_size(true)) {
            member = var.second.get();
        }
    }
    if(member) {
        return member;
    } else {
        return nullptr;
    }
}

uint64_t VariablesContainer::total_byte_size(bool is64Bit) {
    uint64_t size = 0;
    for (const auto &item: variables) {
        auto mem_type = item.second->get_value_type();
        size += mem_type->byte_size(is64Bit);
    }
    return size;
}

void MembersContainer::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    for (const auto &var: variables) {
        var.second->declare_and_link(linker);
    }
    for (const auto &func: functions) {
        func.second->declare_top_level(linker);
        func.second->declare_and_link(linker);
    }
    linker.scope_end();
}

FunctionDeclaration *MembersContainer::member(const std::string &name) {
    auto func = functions.find(name);
    if(func != functions.end()) {
        return func->second.get();
    }
    return nullptr;
}

ASTNode *MembersContainer::child(const std::string &varName) {
    auto found = variables.find(varName);
    if (found != variables.end()) {
        return found->second.get();
    } else {
        auto found_func = functions.find(varName);
        if (found_func != functions.end()) {
            return found_func->second.get();
        } else {
            return nullptr;
        }
    }
}

FunctionDeclaration* MembersContainer::constructor_func(std::vector<std::unique_ptr<Value>>& forArgs) {
    for (const auto & function : functions) {
        if(function.second->has_annotation(AnnotationKind::Constructor) && function.second->satisfy_args(forArgs)) {
            return function.second.get();
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::destructor_func() {
    for (const auto & function : std::ranges::reverse_view(functions)) {
        if(function.second->has_annotation(AnnotationKind::Destructor)) {
            return function.second.get();
        }
    }
    return nullptr;
}

bool MembersContainer::contains_func(FunctionDeclaration* decl) {
    for(auto& function : functions) {
        if(function.second.get() == decl) {
            return true;
        }
    }
    return false;
}

int VariablesContainer::variable_index(const std::string &varName) {
    auto i = 0;
    for (const auto &var: variables) {
        if (var.first == varName) {
            return i;
        }
        i++;
    }
    return -1;
}

void VariablesContainer::declare_and_link(SymbolResolver &linker) {
    for (auto& variable : variables) {
        variable.second->declare_and_link(linker);
    }
}