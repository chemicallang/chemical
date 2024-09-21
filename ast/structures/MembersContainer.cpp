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
#include "ast/types/LinkedType.h"
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
        auto value_type = largest->create_value_type(gen.allocator);
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

std::vector<llvm::Type *> VariablesContainer::elements_type(Codegen &gen, std::vector<ChainValue*>& chain, unsigned index) {
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

void MembersContainer::acquire_function_iterations(int16_t iteration) {
    for(auto& function : functions()) {
        auto& func_data = generic_llvm_data[function];
        if(iteration == func_data.size()) {
            func_data.emplace_back(function->llvm_data);
        } else {
            func_data[iteration] = function->llvm_data;
        }
    }
}

void MembersContainer::early_declare_structural_generic_args(Codegen& gen) {
    for(auto& type_param : generic_params) {
        const auto type = type_param->known_type();
        const auto direct_linked = type->get_direct_linked_node();
        if (direct_linked) {
            auto container = direct_linked->as_members_container();
            if (container) {
                container->code_gen_declare(gen);
            }
        }
    }
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
        return found->second;
    } else {
        return nullptr;
    }
}

BaseDefMember* VariablesContainer::largest_member() {
    BaseDefMember* member = nullptr;
    for(auto& var : variables) {
        if(member == nullptr || var.second->byte_size(true) > member->byte_size(true)) {
            member = var.second;
        }
    }
    return member;
}

uint64_t VariablesContainer::total_byte_size(bool is64Bit) {
    uint64_t size = 0;
    for (const auto &item: variables) {
        size += item.second->byte_size(is64Bit);
//        auto mem_type = item.second->create_value_type();
//        size += mem_type->byte_size(is64Bit);
    }
    return size;
}

void declare_inherited_members(MembersContainer* container, SymbolResolver& linker) {
    for(auto& var : container->variables) {
        var.second->redeclare_top_level(linker, (ASTNode*&) var.second);
    }
    for(auto& func : container->functions()) {
        func->redeclare_top_level(linker, (ASTNode*&) func);
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
        var.second->redeclare_top_level(linker, (ASTNode*&) var.second);
    }
    for(auto& func : functions()) {
        func->redeclare_top_level(linker, (ASTNode*&) func);
    }
}

unsigned int MembersContainer::init_values_req_size() {
    unsigned int i = 0;
    for(auto& inherit : inherited) {
        auto direct = inherit->type->get_direct_linked_struct();
        if(direct && !direct->variables.empty()) {
            i++;
        }
    }
    for(auto& var : variables) {
        if(var.second->default_value() == nullptr) {
            i++;
        }
    }
    return i;
}

void MembersContainer::declare_and_link_no_scope(SymbolResolver &linker) {
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (ASTNode*&) gen_param);
    }
    for(auto& inherits : inherited) {
        inherits->type->link(linker, (BaseType*&) inherits);
        const auto def = inherits->type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
    for (auto &var: variables) {
        var.second->declare_and_link(linker, (ASTNode*&) var.second);
    }
    for(auto& func : functions()) {
        func->declare_top_level(linker, (ASTNode*&) func);
    }
    for (auto &func: functions()) {
        func->declare_and_link(linker, (ASTNode*&) func);
    }
}

void MembersContainer::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
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
        return found->second;
    } else {
        auto found_func = indexes.find(varName);
        if (found_func != indexes.end()) {
            return found_func->second;
        } else {
            return nullptr;
        }
    }
}

BaseDefMember *MembersContainer::direct_variable(const std::string& name) {
    auto found = variables.find(name);
    if (found != variables.end()) {
        return found->second;
    } else {
        return nullptr;
    }
}

ASTNode *MembersContainer::direct_child_member(const std::string& name) {
    auto direct_var = direct_variable(name);
    if(direct_var) return direct_var;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits->type->linked_struct_def();
        if(struct_def && struct_def->name == name) {
            return struct_def;
        }
    }
    return nullptr;
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
    const auto direct_mem = direct_variable(name);
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

int16_t MembersContainer::register_generic_args(SymbolResolver& resolver, std::vector<BaseType*>& types) {
    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
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
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::get_last_fn_annotated(AnnotationKind annot) {
    for (const auto & function : std::ranges::reverse_view(functions())) {
        if(function->has_annotation(annot)) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::constructor_func(std::vector<Value*>& forArgs) {
    for (const auto & function : functions()) {
        if(function->has_annotation(AnnotationKind::Constructor) && function->satisfy_args(forArgs)) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::implicit_constructor_func(Value* value) {
    for (const auto & function : functions()) {
        if(function->has_annotation(AnnotationKind::Implicit) && function->params.size() == 1 && function->params[0]->type->satisfies(value)) {
            return function;
        }
    }
    return nullptr;
}

bool members_type_require(MembersContainer& container, bool(*requirement)(BaseType*)) {
    for(const auto& inh : container.inherited) {
        if(requirement(inh->type)) {
            return true;
        }
    }
    for(const auto& var : container.variables) {
        auto type = var.second->known_type();
        if(requirement(type)) {
            return true;
        }
    }
    return false;
}

bool MembersContainer::any_member_has_destructor() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_destructor() != nullptr;
    });
}

bool MembersContainer::any_member_has_clear_func() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_clear_fn() != nullptr;
    });
}

bool MembersContainer::any_member_has_pre_move_func() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_pre_move_fn() != nullptr;
    });
}

bool MembersContainer::any_member_has_move_func() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_move_fn() != nullptr;
    });
}

bool MembersContainer::any_member_has_copy_func() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_copy_fn() != nullptr;
    });
}

bool MembersContainer::requires_destructor() {
    return destructor_func() != nullptr || any_member_has_destructor();
}

bool MembersContainer::requires_clear_fn() {
    return clear_func() != nullptr || any_member_has_clear_func();
}

bool MembersContainer::requires_move_fn() {
    return pre_move_func() != nullptr || any_member_has_pre_move_func();
}

bool MembersContainer::requires_copy_fn() {
    return copy_func() != nullptr || any_member_has_copy_func();
}

FunctionDeclaration* MembersContainer::pre_move_func() {
    auto post_move = clear_func();
    if(post_move) return nullptr;
    auto move_fn = move_func();
    if(move_fn) return move_fn;
    auto copy_fn = copy_func();
    if(copy_fn && copy_fn->has_annotation(AnnotationKind::Implicit)) {
        return copy_fn;
    }
    return nullptr;
}

void MembersContainer::insert_func(FunctionDeclaration* decl) {
    indexes[decl->name] = decl;
    functions_container.emplace_back(decl);
}

FunctionDeclaration* MembersContainer::create_destructor(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration("delete", {}, new (allocator.allocate<VoidType>()) VoidType(nullptr), false, this, nullptr, std::nullopt);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 0, nullptr, decl, nullptr));
    decl->body.emplace(LoopScope{nullptr, nullptr});
    decl->annotations.emplace_back(AnnotationKind::Delete);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_clear_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration("clear", {}, new VoidType(nullptr), false, this, nullptr, std::nullopt);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 0, nullptr, decl, nullptr));
    decl->body.emplace(LoopScope{nullptr, nullptr});
    decl->annotations.emplace_back(AnnotationKind::Clear);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_copy_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration("copy", {}, new VoidType(nullptr), false, this, nullptr, std::nullopt);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 0, nullptr, decl, nullptr));
    decl->params.emplace_back(new FunctionParam("other", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 1, nullptr, decl, nullptr));
    decl->body.emplace(LoopScope{nullptr, nullptr});
    decl->annotations.emplace_back(AnnotationKind::Copy);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_move_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration("move", {}, new VoidType(nullptr), false, this, nullptr, std::nullopt);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 0, nullptr, decl, nullptr));
    decl->params.emplace_back(new FunctionParam("other", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(ns_node_identifier(), this, nullptr), nullptr), 1, nullptr, decl, nullptr));
    decl->body.emplace(LoopScope{nullptr, nullptr});
    decl->annotations.emplace_back(AnnotationKind::Copy);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_def_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto delFunc = direct_child_function("delete");
    if(delFunc) {
        diagnoser.error("default destructor is created by name 'delete' , a function by name 'delete' already exists, please create a manual function to avoid this", (AnnotableNode*) delFunc);
        return nullptr;
    }
    return create_destructor(allocator);
}

FunctionDeclaration* MembersContainer::create_def_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto moveFn = direct_child_function("clear");
    if(moveFn) {
        diagnoser.error("default move function is created by name 'move', a function by name 'move' already exists, please create a manual function to avoid this", (AnnotableNode*) moveFn);
        return nullptr;
    }
    return create_clear_fn(allocator);
}

FunctionDeclaration* MembersContainer::create_def_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto copyFn = direct_child_function("copy");
    if(copyFn) {
        diagnoser.error("default copy function is created by name 'copy', a function by name 'copy' already exists, please create a manual function to avoid this", (AnnotableNode*) copyFn);
        return nullptr;
    }
    return create_copy_fn(allocator);
}

FunctionDeclaration* MembersContainer::create_def_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto copyFn = direct_child_function("move");
    if(copyFn) {
        diagnoser.error("default move function is created by name 'move', a function by name 'move' already exists, please create a manual function to avoid this", (AnnotableNode*) copyFn);
        return nullptr;
    }
    return create_move_fn(allocator);
}

bool MembersContainer::insert_multi_func(FunctionDeclaration* decl) {
    auto found = indexes.find(decl->name);
    if(found == indexes.end()) {
        insert_func(std::move(decl));
    } else {
        auto result = handle_name_overload_function(decl->name, found->second, decl);
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
                return { parents_size, inherits->type };
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

void VariablesContainer::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    for (auto& variable : variables) {
        variable.second->declare_and_link(linker, (ASTNode*&) variable.second);
    }
}

InheritedType::InheritedType(BaseType* type, AccessSpecifier specifier) : type(type), specifier(specifier) {

}

std::string& InheritedType::ref_type_name() {
    if(type->kind() == BaseTypeKind::Generic) {
        return ((GenericType*) type)->referenced->type;
    } else if(type->kind() == BaseTypeKind::Linked) {
        return ((LinkedType*) type)->type;
    }
#ifdef DEBUG
    throw std::runtime_error("unable to retrieve referenced type name from type " + type->representation());
#else
    std::cerr << "unable to retrieve referenced type name from type " + type->representation() << std::endl;
#endif
}

std::unique_ptr<InheritedType> InheritedType::copy(ASTAllocator& allocator) const {
    return std::make_unique<InheritedType>(type->copy(allocator), specifier);
}