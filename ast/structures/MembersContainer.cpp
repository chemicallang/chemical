// Copyright (c) Qinetik 2024.

#include "MembersContainer.h"

#include <ranges>
#include "compiler/SymbolResolver.h"
#include "StructMember.h"
#include "MultiFunctionNode.h"
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
        vec.emplace_back(var.second->llvm_type(gen));
    }
    return vec;
}

std::vector<llvm::Type *> VariablesContainer::elements_type(Codegen &gen, std::vector<std::unique_ptr<Value>>& chain, unsigned index) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size());
    for (const auto &var: variables) {
        vec.emplace_back(var.second->llvm_chain_type(gen, chain, index + 1));
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
    for(const auto& func : functions()) {
        func->declare_top_level(linker);
    }
    for (const auto &func: functions()) {
        func->declare_and_link(linker);
    }
    linker.scope_end();
}

FunctionDeclaration *MembersContainer::member(const std::string &name) {
    auto func = indexes.find(name);
    if(func != indexes.end()) {
        return func->second;
    }
    return nullptr;
}

ASTNode *MembersContainer::child(const std::string &varName) {
    auto found = variables.find(varName);
    if (found != variables.end()) {
        return found->second.get();
    } else {
        auto found_func = indexes.find(varName);
        if (found_func != indexes.end()) {
            return found_func->second;
        } else {
            return nullptr;
        }
    }
}

FunctionDeclaration* MembersContainer::constructor_func(std::vector<std::unique_ptr<Value>>& forArgs) {
    for (const auto & function : functions()) {
        if(function->has_annotation(AnnotationKind::Constructor) && function->satisfy_args(forArgs)) {
            return function.get();
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::implicit_constructor_func(Value* value) {
    for (const auto & function : functions()) {
        if(function->has_annotation(AnnotationKind::Implicit) && function->params.size() == 1 && function->params[0]->type->satisfies(value)) {
            return function.get();
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::destructor_func() {
    for (const auto & function : std::ranges::reverse_view(functions())) {
        if(function->has_annotation(AnnotationKind::Destructor)) {
            return function.get();
        }
    }
    return nullptr;
}

bool MembersContainer::requires_destructor() {
    auto destructor = destructor_func();
    if(destructor) return true;
    for(const auto& var : variables) {
        if(var.second->requires_destructor()) {
            return true;
        }
    }
    return false;
}

void MembersContainer::insert_func(std::unique_ptr<FunctionDeclaration> decl) {
    indexes[decl->name] = decl.get();
    functions_container.emplace_back(std::move(decl));
}

bool MembersContainer::insert_multi_func(std::unique_ptr<FunctionDeclaration> decl) {
    auto found = indexes.find(decl->name);
    if(found == indexes.end()) {
        insert_func(std::move(decl));
    } else {
        auto result = handle_name_overridable_function(decl->name, found->second, decl.get());
        if(!result.duplicates.empty()) {
            return false;
        } else if(result.new_multi_func_node) {
            multi_nodes.emplace_back(result.new_multi_func_node);
            // storing pointer to MultiFunctionNode as FunctionDeclaration
            // this can create errors, if not handled properly
            indexes[decl->name] = (FunctionDeclaration*) result.new_multi_func_node;
        }
        functions_container.emplace_back(std::move(decl));
    }
    return true;
}

bool MembersContainer::contains_func(const std::string& name) {
    return indexes.find(name) != indexes.end();
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