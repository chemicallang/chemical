// Copyright (c) Qinetik 2024.

#include "MembersContainer.h"

#include <ranges>
#include "compiler/SymbolResolver.h"
#include "StructMember.h"
#include "MultiFunctionNode.h"
#include "FunctionDeclaration.h"
#include "VariablesContainer.h"
#include "ast/values/StructValue.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

bool VariablesContainer::llvm_struct_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &child_name
) {
    auto index = variable_index(child_name, false);
    if (indexes.empty()) {
        indexes.emplace_back(gen.builder->getInt32(0));
    }
    if (index == -1) {
        const auto curr_size = (int) indexes.size();
        int inherit_ind = 0;
        // checking the inherited structs for given child
        for(auto& inherits : inherited) {
            auto linked_def = inherits->linked_struct_def();
            if(linked_def) {
                if(linked_def->add_child_index(gen, indexes, child_name)) {
                    const auto itr = indexes.begin() + curr_size;
                    indexes.insert(itr, gen.builder->getInt32(inherit_ind));
                    return true;
                }
            }
            inherit_ind++;
        }
        return false;
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
    vec.reserve(variables.size() + inherited.size());
    for(const auto &inherits : inherited) {
        if(inherits->linked_struct_def()) {
            vec.emplace_back(inherits->llvm_type(gen));
        }
    }
    for (const auto &var: variables) {
        vec.emplace_back(var.second->llvm_type(gen));
    }
    return vec;
}

std::vector<llvm::Type *> VariablesContainer::elements_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>>& chain, unsigned index) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size() + inherited.size());
    for(const auto &inherits : inherited) {
        if(inherits->linked_struct_def()) {
            vec.emplace_back(inherits->llvm_chain_type(gen, chain, index + 1));
        }
    }
    for (const auto &var: variables) {
        vec.emplace_back(var.second->llvm_chain_type(gen, chain, index + 1));
    }
    return vec;
}

std::pair<llvm::Value*, llvm::FunctionType*>& MembersContainer::llvm_generic_func_data(FunctionDeclaration* decl, int16_t struct_itr, int16_t func_itr) {
    return generic_llvm_data[decl][struct_itr][func_itr];
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

void declare_inherited_members(MembersContainer* container, SymbolResolver& linker) {
    for(auto& var : container->variables) {
        var.second->redeclare_top_level(linker);
    }
    for(auto& func : container->functions()) {
        func->redeclare_top_level(linker);
    }
    for(auto& inherits : container->inherited) {
        const auto def = inherits->linked->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
}

void MembersContainer::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker);
    }
    for(auto& inherits : inherited) {
        const auto def = inherits->linked->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
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

BaseDefMember *MembersContainer::direct_child_member(const std::string& name) {
    auto found = variables.find(name);
    if (found != variables.end()) {
        return found->second.get();
    } else {
        return nullptr;
    }
}

BaseDefMember *MembersContainer::inherited_member(const std::string& name) {
    for(auto& inherits : inherited) {
        const auto struct_def = inherits->linked_struct_def();
        if(struct_def) {
            const auto mem = struct_def->child_member(name);
            if(mem) return mem;
        }
    }
}

BaseDefMember *MembersContainer::child_member(const std::string& name) {
    const auto direct_mem = direct_child_member(name);
    if(direct_mem) return direct_mem;
    const auto inherited_mem = inherited_member(name);
    if(inherited_mem) return inherited_mem;
    return nullptr;
}

FunctionDeclaration *MembersContainer::child_function(const std::string& name) {
    auto found_func = indexes.find(name);
    if (found_func != indexes.end()) {
        return found_func->second;
    } else {
        return nullptr;
    }
}

int16_t MembersContainer::register_generic_args(SymbolResolver& resolver, std::vector<std::unique_ptr<BaseType>>& types) {
    const auto itr = register_generic_usage(resolver, this, generic_params, types);
    if(itr.second) {
        for (auto sub: subscribers) {
            sub->report_parent_usage(resolver, itr.first);
        }
    }
    return itr.first;
}

int16_t MembersContainer::register_value(SymbolResolver& resolver, StructValue* value) {
    return register_generic_args(resolver, value->generic_list);
}

int16_t MembersContainer::total_generic_iterations() {
    return ::total_generic_iterations(generic_params);
}

void MembersContainer::set_active_iteration(int16_t iteration) {
#ifdef DEBUG
    if(iteration < -1) {
        throw std::runtime_error("please fix iteration, which is less than -1, generic iteration is always greater than or equal to -1");
    }
#endif
    if(iteration == -1) {
        active_iteration = 0;
    } else {
        active_iteration = iteration;
    }
    for (auto &param: generic_params) {
        param->active_iteration = iteration;
    }
    for(auto sub : subscribers) {
        sub->set_parent_iteration(iteration);
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

long VariablesContainer::variable_index(const std::string &varName, bool consider_inherited_structs) {
    long parents_size = 0;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits->linked->as_struct_def();
        if(struct_def) {
            if(consider_inherited_structs && struct_def->name == varName) {
                // user wants the struct
                return parents_size;
            }
            parents_size += 1;
        }
    }
    auto found = variables.find(varName);
    if(found == variables.end()) {
        return -1;
    } else {
        return ((long)(found - variables.begin())) + parents_size;
    }
}

long VariablesContainer::direct_child_index(const std::string &varName) {
    auto found = variables.find(varName);
    if(found == variables.end()) {
        return -1;
    } else {
        return ((long)(found - variables.begin()));
    }
}

bool VariablesContainer::build_path_to_child(std::vector<int>& path, const std::string& child_name) {
    const auto child_ind = direct_child_index(child_name);
    if(child_ind != -1) {
        path.emplace_back(child_ind);
        return true;
    }
    auto inherit_index = 0;
    for(auto& inherits : inherited) {
        const auto linked_struct = inherits->linked_struct_def();
        if(linked_struct) {
            const auto curr_size = path.size();
            path.emplace_back(inherit_index);
            auto found = linked_struct->build_path_to_child(path, child_name);
            if(found) {
                return true;
            } else if(curr_size != path.size()){
                path.erase(path.begin() + ((long long) curr_size), path.end());
            }
        }
        inherit_index++;
    }
    return false;
}

void VariablesContainer::declare_and_link(SymbolResolver &linker) {
    for (auto& variable : variables) {
        variable.second->declare_and_link(linker);
    }
}