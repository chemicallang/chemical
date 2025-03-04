// Copyright (c) Chemical Language Foundation 2025.

#include "MembersContainer.h"

#include <ranges>
#include "compiler/SymbolResolver.h"
#include "StructMember.h"
#include "MultiFunctionNode.h"
#include "FunctionDeclaration.h"
#include "VariablesContainer.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
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
        const chem::string_view &child_name
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
            auto linked_def = inherits.type->linked_struct_def();
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
        const chem::string_view &name
) {
    auto largest = largest_member();
    if(largest) {
        auto value_type = largest->create_value_type(gen.allocator);
        // this should only be added if we are not inlining struct types inside union
        if(value_type->isStructLikeType()) {
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
        if(inherits.type->linked_struct_def()) {
            vec.emplace_back(inherits.type->llvm_type(gen));
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
        if(inherits.type->linked_struct_def()) {
            vec.emplace_back(inherits.type->llvm_chain_type(gen, chain, index + 1));
        }
    }
    for (const auto &var: variables) {
        vec.emplace_back(var.second->llvm_chain_type(gen, chain, index + 1));
    }
    return vec;
}

llvm::Function*& MembersContainer::llvm_generic_func_data(FunctionDeclaration* decl, int16_t struct_itr, int16_t func_itr) {
    return generic_llvm_data.at(decl).at(struct_itr).at(func_itr);
}

void MembersContainer::external_declare(Codegen& gen) {
    if(generic_params.empty()) {
        for (auto& function: functions()) {
            function->code_gen_external_declare(gen);
        }
    } else {
        if(functions().empty()) return;
        // get the total number of iterations that already exist for each function
        const auto total_existing_itr = generic_llvm_data[functions().front()].size();
        // clear the existing iteration's llvm_data since that's out of module
        for(auto& func : functions()) {
            generic_llvm_data[func].clear();
        }
        int16_t i = 0;
        const auto prev_active_iteration = active_iteration;
        while(i < total_existing_itr) {
            set_active_iteration(i);
            // declare all the functions at iteration
            for (auto& function: functions()) {
                function->code_gen_external_declare(gen);
            }
            // acquire functions llvm data at iteration
            acquire_function_iterations(i);
            i++;
        }
        set_active_iteration_safely(prev_active_iteration);
        // calling code_gen, this will generate any missing generic iterations
        code_gen_declare(gen);
        code_gen(gen);
    }
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

llvm::Function* MembersContainer::llvm_func_data(FunctionDeclaration* decl) {
    if(!generic_params.empty()) {
        return llvm_generic_func_data(decl, active_iteration, decl->active_iteration);
    } else {
        return decl->llvm_func();
    }
}

void MembersContainer::llvm_build_inherited_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types) {
    for(auto& inherits : inherited) {
        const auto linked = inherits.type->linked_node()->as_interface_def();
        if(linked) {
            linked->llvm_build_inherited_vtable_type(gen, struct_types);
            linked->llvm_vtable_type(gen, struct_types);
        }
    }
}

void MembersContainer::llvm_build_inherited_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    for(auto& inherits : inherited) {
        const auto linked = inherits.type->linked_node()->as_interface_def();
        if(linked) {
            linked->llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
            linked->llvm_build_vtable(gen, for_struct, llvm_pointers);
        }
    }
}

#endif

BaseDefMember *VariablesContainer::child_def_member(const chem::string_view &name) {
    auto found = variables.find(name);
    if (found != variables.end()) {
        return found->second;
    } else {
        return nullptr;
    }
}

ASTNode *VariablesContainer::child_member_or_inherited_struct(const chem::string_view& name) {
    auto direct_var = direct_variable(name);
    if(direct_var) return direct_var;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def && struct_def->name_view().data() == name) {
            return struct_def;
        }
    }
    return nullptr;
}

BaseDefMember *VariablesContainer::inherited_member(const chem::string_view& name) {
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->linked_struct_def();
        if(struct_def) {
            const auto mem = struct_def->child_member(name);
            if(mem) return mem;
        }
    }
    return nullptr;
}

BaseDefMember *VariablesContainer::child_member(const chem::string_view& name) {
    const auto direct_mem = direct_variable(name);
    if(direct_mem) return direct_mem;
    const auto inherited_mem = inherited_member(name);
    if(inherited_mem) return inherited_mem;
    return nullptr;
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
    size_t offset = 0;
    size_t maxAlignment = 1;
    for (const auto& member : variables) {
        // Update max alignment
        const auto member_alignment = (size_t) member.second->known_type()->type_alignment(is64Bit);
        maxAlignment = std::max(maxAlignment, member_alignment);
        // Align the current offset
        size_t padding = (member_alignment - (offset % member_alignment)) % member_alignment;
        offset += padding;

        // Add the size of the member
        offset += member.second->byte_size(is64Bit);
    }
    // Align the total size to the largest alignment
    size_t totalPadding = (maxAlignment - (offset % maxAlignment)) % maxAlignment;
    offset += totalPadding;
    return offset;
}

uint64_t VariablesContainer::largest_member_byte_size(bool is64Bit) {
    uint64_t size = 0;
    uint64_t previous;
    for (auto &mem: variables) {
        previous = mem.second->byte_size(is64Bit);
        if (previous > size) {
            size = previous;
        }
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
        const auto def = inherits.type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
}

void MembersContainer::redeclare_inherited_members(SymbolResolver &linker) {
    for(auto& inherits : inherited) {
        const auto def = inherits.type->linked_node()->as_members_container();
        if(def) {
            declare_inherited_members(def, linker);
        }
    }
}

void MembersContainer::redeclare_variables_and_functions(SymbolResolver &linker) {
    for (auto &var: variables) {
        var.second->redeclare_top_level(linker);
    }
    for(auto& func : functions()) {
        func->redeclare_top_level(linker);
    }
}

unsigned int MembersContainer::init_values_req_size() {
    unsigned int i = 0;
    for(auto& inherit : inherited) {
        auto direct = inherit.type->get_direct_linked_struct();
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

void MembersContainer::link_signature(SymbolResolver &linker) {
    linker.scope_start();
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (ASTNode*&) gen_param);
    }
    for(auto& inherits : inherited) {
        inherits.type->link(linker);
    }
    for (auto &var: variables) {
        var.second->declare_and_link(linker, (ASTNode*&) var.second);
    }
    linker.scope_end();
}

void MembersContainer::declare_and_link_no_scope(SymbolResolver &linker) {
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (ASTNode*&) gen_param);
    }
    for(auto& inherits : inherited) {
        // TODO remove this type linking, since we do this in link_signature
        inherits.type->link(linker);
        const auto def = inherits.type->get_members_container();
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
    for(auto& func : functions()) {
        func->link_signature(linker);
    }
    for (auto& func: functions()) {
        func->declare_and_link(linker, (ASTNode*&) func);
    }
    if(is_generic()) {
        // reporting iterations to subscribers
        // WHY ? well, this container may have been used before it was linked (only signature linked)
        // in files above, when functions inside the container aren't linked, meaning we
        // have zero subscribers that are present inside the body of the functions that are in this container
        const auto t = total_generic_iterations();
        int16_t i = 0;
        while (i < t) {
            report_iteration_to_subs(*linker.ast_allocator, linker, i);
            i++;
        }
    }
}

void MembersContainer::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    declare_and_link_no_scope(linker);
    linker.scope_end();
}

void MembersContainer::register_use_to_inherited_interfaces(StructDefinition* definition) {
    for(auto& inherits : inherited) {
        const auto interface = inherits.type->linked_interface_def();
        if(interface) {
            interface->register_use(definition);
            interface->register_use_to_inherited_interfaces(definition);
        }
    }
}

FunctionDeclaration *MembersContainer::member(const chem::string_view &name) {
    auto func = indexes.find(name);
    if(func != indexes.end()) {
        return func->second;
    }
    return nullptr;
}

ASTNode *MembersContainer::child(const chem::string_view &varName) {
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

FunctionDeclaration* MembersContainer::inherited_function(const chem::string_view& name) {
    for(auto& inherits : inherited) {
        const auto linked = inherits.type->get_direct_linked_node();
        const auto container = linked ? linked->as_members_container() : nullptr;
        if(container) {
            const auto func = container->direct_child_function(name);
            if(func) return func;
            const auto func2 = container->inherited_function(name);
            return func2;
        }
    }
    return nullptr;
}

FunctionDeclaration *MembersContainer::direct_child_function(const chem::string_view& name) {
    auto found_func = indexes.find(name);
    if (found_func != indexes.end()) {
        return found_func->second;
    } else {
        return nullptr;
    }
}

FunctionOverridingInfo MembersContainer::get_func_overriding_info(FunctionDeclaration* function) {
    if(inherited.empty()) return { nullptr, nullptr, nullptr };
    for(auto& inherits : inherited) {
        auto& type = *inherits.type;
        const auto linked_node = type.get_direct_linked_node();
        if(linked_node) {
            const auto linked_node_kind = linked_node->kind();
            if (linked_node_kind == ASTNodeKind::InterfaceDecl) {
                const auto interface = linked_node->as_interface_def_unsafe();
                const auto child_func = interface->direct_child_function(function->name_view());
                if (child_func) {
                    return { &inherits, interface, child_func};
                } else {
                    continue;
                }
            } else if (linked_node_kind == ASTNodeKind::StructDecl) {
                const auto struct_def = linked_node->as_struct_def_unsafe();
                const auto child_func = struct_def->direct_child_function(function->name_view());
                if (child_func) {
                    return { &inherits, struct_def, child_func};
                } else {
                    const auto info = struct_def->get_func_overriding_info(function);
                    if (info.type) {
                        return info;
                    }
                }
            }
        }
    }
    return { nullptr, nullptr, nullptr };
}

std::pair<ASTNode*, FunctionDeclaration*> MembersContainer::get_func_with_signature(FunctionDeclaration* function) {
    auto direct = direct_child_function(function->name_view());
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

int16_t MembersContainer::register_with_existing(ASTDiagnoser& diagnoser, std::vector<BaseType*>& types) {
    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }
    const auto itr = get_iteration_for(generic_params, generic_args);
    if(itr == -1) {
        diagnoser.warn("iteration for declaration doesn't exist", (ASTNode*) this);
    }
    return itr;
}

void MembersContainer::report_iteration_to_subs(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, int16_t itr) {
    for (auto sub: subscribers) {
        if(sub->referenced->linked == this) {
            continue;
        }
        sub->report_parent_usage(astAllocator, diagnoser, itr);
    }
    for(const auto func : functions()) {
        func->register_parent_iteration(astAllocator, diagnoser, itr);
    }
}

int16_t MembersContainer::register_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& types) {
    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }
    const auto itr = register_generic_usage(astAllocator, generic_params, generic_args);
    set_active_iteration_no_subs(itr.first);
    if(itr.second) {
        report_iteration_to_subs(astAllocator, diagnoser, itr.first);
    }
    return itr.first;
}

int16_t MembersContainer::register_value(SymbolResolver& resolver, StructValue* value) {
    auto gen_list = value->create_generic_list();
    return register_generic_args(*resolver.ast_allocator, resolver, gen_list);
}

int16_t MembersContainer::total_generic_iterations() {
    return ::total_generic_iterations(generic_params);
}

void MembersContainer::set_active_iteration_no_subs(int16_t iteration) {
#ifdef DEBUG
    if(iteration < -1) {
        throw std::runtime_error("please fix iteration, which is less than -1, generic iteration is always greater than or equal to -1");
    }
#endif
    if(is_generic()) {
        active_iteration = iteration;
        for (auto& param: generic_params) {
            param->set_active_iteration(iteration);
        }
    }
    for(auto& inh : inherited) {
        inh.type->set_generic_iteration(inh.type->get_generic_iteration());
    }
}

void MembersContainer::set_active_iteration(int16_t iteration) {
#ifdef DEBUG
    if(iteration < -1) {
        throw std::runtime_error("please fix iteration, which is less than -1, generic iteration is always greater than or equal to -1");
    }
#endif
    if(is_generic()) {
        active_iteration = iteration;
        for (auto& param: generic_params) {
            param->set_active_iteration(iteration);
        }
        for (auto sub: subscribers) {
            sub->set_parent_iteration(iteration);
        }
    }
    for(auto& inh : inherited) {
        inh.type->set_generic_iteration(inh.type->get_generic_iteration());
    }
}

FunctionDeclaration* MembersContainer::get_first_constructor() {
    for(const auto function : functions()) {
        if(function->is_constructor_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::destructor_func() {
    for (const auto function : std::ranges::reverse_view(functions())) {
        if(function->is_delete_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::clear_func() {
    for (const auto function : std::ranges::reverse_view(functions())) {
        if(function->is_post_move_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::move_func() {
    for (const auto function : std::ranges::reverse_view(functions())) {
        if(function->is_move_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::copy_func() {
    for (const auto function : std::ranges::reverse_view(functions())) {
        if(function->is_copy_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::default_constructor_func() {
    for(const auto function : functions()) {
        if(function->is_constructor_fn() && function->params.empty()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::constructor_func(ASTAllocator& allocator, std::vector<Value*>& forArgs) {
    for (const auto function : functions()) {
        if(function->is_constructor_fn() && function->satisfy_args(allocator, forArgs)) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::implicit_constructor_func(ASTAllocator& allocator, Value* value) {
    for (const auto & function : functions()) {
        if(function->is_implicit() && function->params.size() == 1 && function->params[0]->type->satisfies(allocator, value, false)) {
            return function;
        }
    }
    return nullptr;
}

bool variables_type_require(VariablesContainer& container, bool(*requirement)(BaseType*), bool variant_container) {
    for(const auto& inh : container.inherited) {
        if(requirement(inh.type)) {
            return true;
        }
    }
    if(variant_container) {
        for(const auto& var : container.variables) {
            const auto mem = var.second->as_variant_member_unsafe();
            for(auto& val : mem->values) {
                if(requirement(val.second->type)) {
                    return true;
                }
            }
        }
        return false;
    } else {
        for(const auto& var : container.variables) {
            const auto k = var.second->kind();
            switch(k) {
                case ASTNodeKind::UnnamedStruct:
                    if(variables_type_require(*var.second->as_unnamed_struct_unsafe(), requirement, false)){
                        return true;
                    }
                    break;
                case ASTNodeKind::UnnamedUnion:
                    if(variables_type_require(*var.second->as_unnamed_union_unsafe(), requirement, false)){
                        return true;
                    }
                    break;
                case ASTNodeKind::StructMember:
                    if(requirement(var.second->as_struct_member_unsafe()->type)) {
                        return true;
                    }
                    break;
                default:
                    break;
            }
        }
        return false;
    }
}

inline bool members_type_require(MembersContainer& container, bool(*requirement)(BaseType*)) {
    return variables_type_require(container, requirement, ASTNode::isVariantDecl(container.kind()));
}

bool MembersContainer::any_member_has_def_constructor() {
    return members_type_require(*this, [](BaseType* type)-> bool {
        return type->get_def_constructor() != nullptr;
    });
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

FunctionDeclaration* MembersContainer::pre_move_func() {
    auto post_move = clear_func();
    if(post_move) return nullptr;
    auto move_fn = move_func();
    if(move_fn) return move_fn;
    auto copy_fn = copy_func();
    if(copy_fn && copy_fn->is_implicit()) {
        return copy_fn;
    }
    return nullptr;
}

void MembersContainer::insert_func(FunctionDeclaration* decl) {
    indexes[decl->name_view()] = decl;
    functions_container.emplace_back(decl);
}

void MembersContainer::insert_functions(const std::initializer_list<FunctionDeclaration*>& decls) {
    for(const auto d : decls) {
        indexes[d->name_view()] = d;
        functions_container.emplace_back(d);
    }
}

FunctionDeclaration* MembersContainer::create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name) {
    const auto returnType = new (allocator.allocate<LinkedType>()) LinkedType(parent_name, this, ZERO_LOC);
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("make"), returnType, false, this, ZERO_LOC);
    decl->body.emplace(Scope{nullptr, ZERO_LOC});
    decl->set_constructor_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_destructor(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("delete"), new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC), false, this, ZERO_LOC);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 0, nullptr, true, decl, ZERO_LOC));
    decl->body.emplace(Scope{nullptr, ZERO_LOC});
    decl->set_delete_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_clear_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("clear"), new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC), false, this, ZERO_LOC);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 0, nullptr, true, decl, ZERO_LOC));
    decl->body.emplace(Scope{nullptr, ZERO_LOC});
    decl->set_clear_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_copy_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("copy"), new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC), false, this, ZERO_LOC);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 0, nullptr, true, decl, ZERO_LOC));
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("other", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 1, nullptr, true, decl, ZERO_LOC));
    decl->body.emplace(Scope{nullptr, ZERO_LOC});
    decl->set_copy_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_move_fn(ASTAllocator& allocator) {
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("move"), new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC), false, this, ZERO_LOC);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 0, nullptr, true, decl, ZERO_LOC));
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("other", new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(get_located_id()->identifier, this, ZERO_LOC), ZERO_LOC, true), 1, nullptr, true, decl, ZERO_LOC));
    decl->body.emplace(Scope{nullptr, ZERO_LOC});
    decl->set_move_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_def_constructor_checking(ASTAllocator& allocator, ASTDiagnoser& diagnoser, const chem::string_view& container_name) {
    auto delFunc = direct_child_function("make");
    if(delFunc) {
        diagnoser.error("default constructor is created by name 'make' , a function by name 'make' already exists, please create a manual constructor to avoid this", (AnnotableNode*) delFunc);
        return nullptr;
    }
    return create_def_constructor(allocator, container_name);
}

FunctionDeclaration* MembersContainer::create_def_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto delFunc = direct_child_function("delete");
    if(delFunc) {
        diagnoser.error("default destructor is created by name 'delete', a function by name 'delete' already exists, please create a manual destructor to avoid this", (AnnotableNode*) delFunc);
        return nullptr;
    }
    return create_destructor(allocator);
}

FunctionDeclaration* MembersContainer::create_def_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto moveFn = direct_child_function("clear");
    if(moveFn) {
        diagnoser.error("default post move function is created by name 'postmove', a function by name 'postmove' already exists, please create a manual postmove function to avoid this", (AnnotableNode*) moveFn);
        return nullptr;
    }
    return create_clear_fn(allocator);
}

FunctionDeclaration* MembersContainer::create_def_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto copyFn = direct_child_function("copy");
    if(copyFn) {
        diagnoser.error("default copy function is created by name 'copy', a function by name 'copy' already exists, please create a manual copy function to avoid this", (AnnotableNode*) copyFn);
        return nullptr;
    }
    return create_copy_fn(allocator);
}

FunctionDeclaration* MembersContainer::create_def_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    auto copyFn = direct_child_function("move");
    if(copyFn) {
        diagnoser.error("default move function is created by name 'move', a function by name 'move' already exists, please create a manual move function to avoid this", (AnnotableNode*) copyFn);
        return nullptr;
    }
    return create_move_fn(allocator);
}

bool MembersContainer::insert_multi_func(FunctionDeclaration* decl) {
    auto found = indexes.find(decl->name_view());
    if(found == indexes.end()) {
        insert_func(decl);
    } else {
        auto result = handle_name_overload_function(decl->name_view(), found->second, decl);
        if(!result.duplicates.empty()) {
            return false;
        } else if(result.new_multi_func_node) {
            multi_nodes.emplace_back(result.new_multi_func_node);
            // storing pointer to MultiFunctionNode as FunctionDeclaration
            // this can create errors, if not handled properly
            indexes[decl->name_view()] = (FunctionDeclaration*) result.new_multi_func_node;
        }
        functions_container.emplace_back(decl);
    }
    return true;
}

bool MembersContainer::contains_func(const chem::string_view& name) {
    return indexes.find(name) != indexes.end();
}

BaseType* MembersContainer::create_linked_type(const chem::string_view& name, ASTAllocator& allocator) {
    const auto linked_type = new (allocator.allocate<LinkedType>()) LinkedType(name, this, ZERO_LOC);
    if(generic_params.empty()) {
        return linked_type;
    } else {
        const auto gen_type = new (allocator.allocate<GenericType>()) GenericType(linked_type, {});
//        if(active_iteration == 0) {
            // strut Thing<T> => Thing<T> where T references is linked with original type parameter T
            for (auto& param: generic_params) {
                gen_type->types.emplace_back(new(allocator.allocate<LinkedType>()) LinkedType(param->identifier, param, ZERO_LOC));
            }
//        } else {
//            for (auto& param: generic_params) {
//                gen_type->types.emplace_back(param->known_type());
//            }
//        }
        return gen_type;
    }
}

bool MembersContainer::extends_node(ASTNode* other) {
    if(!inherited.empty()) {
        const auto otherKind = other->kind();
        if (otherKind == ASTNodeKind::StructDecl) {
            const auto inherited_node = inherited.front().type->get_direct_linked_node();
            if(!inherited_node) return false;
            if(inherited_node == other) {
                return true;
            } else {
                const auto container = inherited_node->get_members_container();
                return container && container->extends_node(other);
            }
        } else if(otherKind == ASTNodeKind::InterfaceDecl) {
            for(auto& inh : inherited) {
                const auto inherited_node = inh.type->get_direct_linked_node();
                if(!inherited_node) return false;
                const auto container = inherited_node->get_members_container();
                if(container && container->extends_node(other)) {
                    return true;
                }
            }
            return false;
        }
    } else {
        return false;
    }
}

std::pair<long, BaseType*> VariablesContainer::variable_type_index(const chem::string_view& varName, bool consider_inherited_structs) {
    long parents_size = 0;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->linked_node()->as_struct_def();
        if(struct_def) {
            if(consider_inherited_structs && struct_def->name_view().data() == varName) {
                // user wants the struct
                return { parents_size, inherits.type };
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

long VariablesContainer::direct_child_index(const chem::string_view& varName) {
    auto found = variables.find(varName);
    if(found == variables.end()) {
        return -1;
    } else {
        return ((long)(found - variables.begin()));
    }
}

bool VariablesContainer::does_override(InterfaceDefinition* interface) {
    for(auto& inherits : inherited) {
        const auto container = inherits.type->linked_node()->as_variables_container();
        if(container == interface || container->does_override(interface)) {
            return true;
        }
    }
    return false;
}

bool VariablesContainer::build_path_to_child(std::vector<int>& path, const chem::string_view& child_name) {
    const auto child_ind = direct_child_index(child_name);
    if(child_ind != -1) {
        path.emplace_back(child_ind);
        return true;
    }
    auto inherit_index = 0;
    for(auto& inherits : inherited) {
        const auto linked_struct = inherits.type->linked_struct_def();
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

const chem::string_view& InheritedType::ref_type_name() {
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

InheritedType InheritedType::copy(ASTAllocator& allocator) const {
    return InheritedType(type->copy(allocator), specifier);
}