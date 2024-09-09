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
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"

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
            auto linked_def = inherits->type->linked_struct_def();
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
        if(inherits->type->linked_struct_def()) {
            vec.emplace_back(inherits->type->llvm_type(gen));
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
        if(inherits->type->linked_struct_def()) {
            vec.emplace_back(inherits->type->llvm_chain_type(gen, chain, index + 1));
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

std::pair<llvm::Value*, llvm::FunctionType*> MembersContainer::llvm_func_data(FunctionDeclaration* decl) {
    std::pair<llvm::Value*, llvm::FunctionType*> data;
    if(!generic_params.empty()) {
        const auto llvm_data = llvm_generic_func_data(decl, active_iteration, decl->active_iteration);
        data.second = llvm_data.second;
        data.first = llvm_data.first;
    } else {
        data.second = decl->known_func_type();
        data.first = decl->llvm_callee();
    }
    return data;
}

void MembersContainer::llvm_build_inherited_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types) {
    for(auto& inherits : inherited) {
        const auto linked = inherits->type->linked_node()->as_interface_def();
        if(linked) {
            linked->llvm_build_inherited_vtable_type(gen, struct_types);
            linked->llvm_vtable_type(gen, struct_types);
        }
    }
}

void MembersContainer::llvm_build_inherited_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    for(auto& inherits : inherited) {
        const auto linked = inherits->type->linked_node()->as_interface_def();
        if(linked) {
            linked->llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
            linked->llvm_build_vtable(gen, for_struct, llvm_pointers);
        }
    }
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
    return member;
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
        var.second->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) var.second);
    }
    for(auto& func : container->functions()) {
        func->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) func);
    }
    for(auto& inherits : container->inherited) {
        const auto def = inherits->type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
}

void MembersContainer::redeclare_inherited_members(SymbolResolver &linker) {
    for(auto& inherits : inherited) {
        const auto def = inherits->type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
}

void MembersContainer::redeclare_variables_and_functions(SymbolResolver &linker) {
    for (auto &var: variables) {
        var.second->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) var.second);
    }
    for(auto& func : functions()) {
        func->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) func);
    }
}

void MembersContainer::declare_and_link_no_scope(SymbolResolver &linker) {
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) gen_param);
    }
    for(auto& inherits : inherited) {
        inherits->type->link(linker, (std::unique_ptr<BaseType>&) inherits);
        const auto def = inherits->type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
    for (auto &var: variables) {
        var.second->declare_and_link(linker, (std::unique_ptr<ASTNode>&) var.second);
    }
    for(auto& func : functions()) {
        func->declare_top_level(linker, (std::unique_ptr<ASTNode>&) func);
    }
    for (auto &func: functions()) {
        func->declare_and_link(linker, (std::unique_ptr<ASTNode>&) func);
    }
}

void MembersContainer::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.scope_start();
    declare_and_link_no_scope(linker);
    linker.scope_end();
}

void MembersContainer::register_use_to_inherited_interfaces(StructDefinition* definition) {
    for(auto& inherits : inherited) {
        const auto interface = inherits->type->linked_node()->as_interface_def();
        if(interface) {
            interface->register_use(definition);
            interface->register_use_to_inherited_interfaces(definition);
        }
    }
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
        const auto struct_def = inherits->type->linked_struct_def();
        if(struct_def) {
            const auto mem = struct_def->child_member(name);
            if(mem) return mem;
        }
    }
    return nullptr;
}

BaseDefMember *MembersContainer::child_member(const std::string& name) {
    const auto direct_mem = direct_child_member(name);
    if(direct_mem) return direct_mem;
    const auto inherited_mem = inherited_member(name);
    if(inherited_mem) return inherited_mem;
    return nullptr;
}

FunctionDeclaration *MembersContainer::direct_child_function(const std::string& name) {
    auto found_func = indexes.find(name);
    if (found_func != indexes.end()) {
        return found_func->second;
    } else {
        return nullptr;
    }
}

// returns the struct/interface & function that is being overridden by given function in the parameter
std::pair<ASTNode*, FunctionDeclaration*> MembersContainer::get_overriding_info(FunctionDeclaration* function) {
    if(inherited.empty()) return { nullptr, nullptr };
    for(auto& inherits : inherited) {
        const auto interface = inherits->type->linked_node()->as_interface_def();
        if(interface) {
            const auto child_func = interface->direct_child_function(function->name);
            if(child_func) {
                return { interface, child_func };
            } else {
                continue;
            }
        }
        const auto struct_def = inherits->type->linked_node()->as_struct_def();
        if(struct_def) {
            const auto child_func = struct_def->direct_child_function(function->name);
            if(child_func) {
                return {struct_def, child_func};
            } else {
                const auto info = struct_def->get_overriding_info(function);
                if(info.first) {
                    return info;
                }
            }
        }
    }
    return { nullptr, nullptr };
}

std::pair<ASTNode*, FunctionDeclaration*> MembersContainer::get_func_with_signature(FunctionDeclaration* function) {
    auto direct = direct_child_function(function->name);
    if(direct) return { this, direct };
    return get_overriding_info(function);
}

// returns the function that is being overridden by given function in the parameter
FunctionDeclaration* MembersContainer::get_overriding(FunctionDeclaration* function) {
    return get_overriding_info(function).second;
};

std::pair<InterfaceDefinition*, FunctionDeclaration*> MembersContainer::get_interface_overriding_info(FunctionDeclaration* function) {
    const auto info = get_overriding_info(function);
    const auto interface = info.first ? info.first->as_interface_def() : nullptr;
    if(interface) {
        return { interface, info.second };
    } else {
        return { nullptr, nullptr };
    }
}

InterfaceDefinition* MembersContainer::get_overriding_interface(FunctionDeclaration* function) {
    const auto info = get_overriding_info(function);
    return info.first ? info.first->as_interface_def() : nullptr;
}

int16_t MembersContainer::register_generic_args(SymbolResolver& resolver, std::vector<std::unique_ptr<BaseType>>& types) {
    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type.get();
        i++;
    }
    const auto itr = register_generic_usage(resolver, this, generic_params, generic_args);
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

FunctionDeclaration* MembersContainer::get_first_fn_annotated(AnnotationKind annot) {
    for(const auto & function : functions()) {
        if(function->has_annotation(annot)) {
            return function.get();
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::get_last_fn_annotated(AnnotationKind annot) {
    for (const auto & function : std::ranges::reverse_view(functions())) {
        if(function->has_annotation(annot)) {
            return function.get();
        }
    }
    return nullptr;
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

bool MembersContainer::requires_move_fn() {
    auto move_fn = move_func();
    if(move_fn) return true;
    for(const auto& var : variables) {
        if(var.second->requires_move_fn()) {
            return true;
        }
    }
    return false;
}

void MembersContainer::insert_func(std::unique_ptr<FunctionDeclaration> decl) {
    indexes[decl->name] = decl.get();
    functions_container.emplace_back(std::move(decl));
}

FunctionDeclaration* MembersContainer::create_destructor() {
    auto decl = new FunctionDeclaration("delete", {}, std::make_unique<VoidType>(nullptr), false, this, nullptr, std::nullopt);
    decl->params.emplace_back(new FunctionParam("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(ns_node_identifier(), this, nullptr), nullptr), 0, nullptr, decl, nullptr));
    decl->body.emplace(LoopScope{nullptr, nullptr});
    decl->annotations.emplace_back(AnnotationKind::Destructor);
    insert_func(std::unique_ptr<FunctionDeclaration>(decl));
    return decl;
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

std::pair<long, BaseType*> VariablesContainer::variable_type_index(const std::string &varName, bool consider_inherited_structs) {
    long parents_size = 0;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits->type->linked_node()->as_struct_def();
        if(struct_def) {
            if(consider_inherited_structs && struct_def->name == varName) {
                // user wants the struct
                return { parents_size, inherits->type.get() };
            }
            parents_size += 1;
        }
    }
    auto found = variables.find(varName);
    if(found == variables.end()) {
        return { -1, nullptr };
    } else {
        return { ((long)(found - variables.begin())) + parents_size, found->second->known_type() };
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

bool VariablesContainer::does_override(InterfaceDefinition* interface) {
    for(auto& inherits : inherited) {
        const auto container = inherits->type->linked_node()->as_variables_container();
        if(container == interface || container->does_override(interface)) {
            return true;
        }
    }
    return false;
}

bool VariablesContainer::build_path_to_child(std::vector<int>& path, const std::string& child_name) {
    const auto child_ind = direct_child_index(child_name);
    if(child_ind != -1) {
        path.emplace_back(child_ind);
        return true;
    }
    auto inherit_index = 0;
    for(auto& inherits : inherited) {
        const auto linked_struct = inherits->type->linked_struct_def();
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

void VariablesContainer::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    for (auto& variable : variables) {
        variable.second->declare_and_link(linker, (std::unique_ptr<ASTNode>&) variable.second);
    }
}

InheritedType::InheritedType(std::unique_ptr<BaseType> type, AccessSpecifier specifier) : type(std::move(type)), specifier(specifier) {

}

std::string& InheritedType::ref_type_name() {
    if(type->kind() == BaseTypeKind::Generic) {
        return ((GenericType*) type.get())->referenced->type;
    } else if(type->kind() == BaseTypeKind::Referenced) {
        return ((ReferencedType*) type.get())->type;
    }
#ifdef DEBUG
    throw std::runtime_error("unable to retrieve referenced type name from type " + type->representation());
#else
    std::cerr << "unable to retrieve referenced type name from type " + type->representation() << std::endl;
#endif
}

InheritedType *InheritedType::copy() const {
    return new InheritedType(std::unique_ptr<BaseType>(type->copy()), specifier);
}