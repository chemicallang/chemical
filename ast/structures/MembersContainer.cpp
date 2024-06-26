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

bool MembersContainer::add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &child_name
) {
    auto index = child_index(child_name);
    if (index == -1) {
        return false;
    }
    if (indexes.empty()) {
        indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
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
    for (const auto &field: functions) {
        auto decl = field.second->as_function();
        if (decl != nullptr && decl->name == name) {
            return decl;
        }
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

int MembersContainer::child_index(const std::string &varName) {
    auto i = 0;
    for (const auto &var: variables) {
        if (var.first == varName) {
            return i;
        }
        i++;
    }
    return -1;
}