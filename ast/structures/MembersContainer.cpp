// Copyright (c) Qinetik 2024.

#include "MembersContainer.h"

#include <ranges>
#include "compiler/Codegen.h"
#include "StructMember.h"
#include "FunctionDeclaration.h"

#ifdef COMPILER_BUILD

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

#endif

void MembersContainer::declare_and_link(SymbolResolver &linker) {
    for (const auto &var: variables) {
        var.second->declare_and_link(linker);
    }
    for (const auto &func: functions) {
        func.second->declare_and_link(linker);
    }
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
    Annotation* ann;
    for (const auto & function : std::ranges::reverse_view(functions)) {
        ann = function.second->get_annotation(AnnotationKind::Destructor);
        if(ann) {
            return function.second.get();
        }
    }
    return nullptr;
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

std::string MembersContainer::representation() const {
    std::string ret;
    int i = 0;
    for (const auto &field: variables) {
        ret.append(field.second->representation());
        if (i < variables.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    if(!functions.empty()) {
        ret.append(1, '\n');
    }
    i = 0;
    for (const auto &field: functions) {
        ret.append(field.second->representation());
        if (i < functions.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    return ret;
}