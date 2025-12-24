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
#include "ast/structures/UnsafeBlock.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/statements/AliasStmt.h"
#include "ast/structures/If.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "compiler/symres/DeclareTopLevel.h"
#include <iostream>

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
            auto linked_def = inherits.type->get_direct_linked_struct();
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
        auto value_type = largest->known_type();
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
    vec.reserve(variables().size() + inherited.size());
    for(const auto &inherits : inherited) {
        if(inherits.type->get_direct_linked_struct()) {
            vec.emplace_back(inherits.type->llvm_type(gen));
        }
    }
    for (auto &var: variables()) {
        vec.emplace_back(var->llvm_type(gen));
    }
    return vec;
}

std::vector<llvm::Type *> VariablesContainer::elements_type(Codegen &gen, std::vector<ChainValue*>& chain, unsigned index) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables().size() + inherited.size());
    for(const auto &inherits : inherited) {
        if(inherits.type->get_direct_linked_struct()) {
            vec.emplace_back(inherits.type->llvm_chain_type(gen, chain, index + 1));
        }
    }
    if(index + 1 < chain.size()) {
        const auto linked = ((Value*) chain[index + 1])->linked_node();
        for (const auto var: variables()) {
            if(linked == var) {
                vec.emplace_back(var->llvm_chain_type(gen, chain, index + 1));
            } else {
                vec.emplace_back(var->llvm_type(gen));
            }
        }
    } else {
        for (const auto var: variables()) {
            vec.emplace_back(var->llvm_type(gen));
        }
    }

    return vec;
}

void MembersContainer::external_declare(Codegen& gen) {
    for(const auto func : functions()) {
        switch(func->kind()) {
            default:
                // TODO: multi function nodes could contain generics, which should be handled
                func->code_gen_external_declare(gen);
                break;
            case ASTNodeKind::GenericFuncDecl:{
                // we only external declare the instantiations that have been bodied
                // because otherwise we'd end up with declarations and definitions in the same module
                auto start = func->as_gen_func_decl_unsafe()->instantiations.data();
                const auto end = start + func->as_gen_func_decl_unsafe()->total_bodied_instantiations;
                while(start != end) {
                    (*start)->code_gen_external_declare(gen);
                    start++;
                }
                break;
            }
        }
    }
}

void MembersContainer::llvm_build_inherited_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types) {
    for(auto& inherits : inherited) {
        const auto linked = inherits.type->get_direct_linked_interface();
        if(linked) {
            linked->llvm_build_inherited_vtable_type(gen, struct_types);
            linked->llvm_vtable_type(gen, struct_types);
        }
    }
}

void MembersContainer::llvm_build_inherited_vtable(Codegen& gen, ExtendableMembersContainerNode* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    for(auto& inherits : inherited) {
        const auto linked = inherits.type->get_direct_linked_interface();
        if(linked) {
            linked->llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
            linked->llvm_build_vtable(gen, for_struct, llvm_pointers);
        }
    }
}

llvm::Function* declared_func(Codegen& gen, InterfaceDefinition* def, FunctionDeclaration* func) {
    const auto ptr = func->llvm_func(gen);
    if(ptr == nullptr) {
        func->code_gen_declare(gen, def);
        return func->llvm_func(gen);
    }
    return ptr;
}

void ExtendableMembersContainerNode::llvm_override_declare(Codegen& gen, FunctionDeclaration* function) {
    const auto inherited_func = inherited_function(function->name_view());
    if(inherited_func == nullptr) {
        gen.error("couldn't find base function", function);
        return;
    }
    const auto func_parent = inherited_func->parent();
    if(func_parent == nullptr || func_parent->kind() != ASTNodeKind::InterfaceDecl) {
        gen.error("couldn't find base function container", function);
        return;
    }
    const auto interface = func_parent->as_interface_def_unsafe();
    // we always assume base container as interface, it could be something else (abstract struct maybe)
    if(interface->is_static()) {
        const auto func = declared_func(gen, interface, inherited_func);
        const auto interfaceModule = func->getParent();
        if(interfaceModule != gen.module.get()) {
            // interface is present in another module
            // we create a new function with strong linkage in this module
            const auto new_func = gen.create_function(func->getName(), func->getFunctionType(), function, AccessSpecifier::Public);
            function->set_llvm_data(gen, new_func);
        } else {
            // internal interface, present in current module
            // we will implement the interface in place, since its present in current module
            function->set_llvm_data(gen, func);
            if(func->size() == 1) {
                // remove the stub block present in functions internal to module
                auto& stubEntry = func->getEntryBlock();
                stubEntry.removeFromParent();
            }
            const auto final_specifier = is_linkage_public(interface->specifier()) || is_linkage_public(specifier()) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
            // change the function's linkage to internal
            func->setLinkage(final_specifier);
            gen.createFunctionBlock(func);
        }
    } else {
        auto user_itr = interface->users.find(this);
        if(user_itr == interface->users.end()) {
            gen.error("failed to override function, couldn't find given struct as user of the interface", function);
            return;
        }
        auto& user = user_itr.value();
        auto llvm_data = user.find(inherited_func);
        if (llvm_data == user.end()) {
            // interface hasn't been encountered yet, it comes after the struct
            inherited_func->code_gen_declare(gen, interface);
            const auto func_ptr = inherited_func->known_func(gen);
            user[inherited_func] = { func_ptr, true };
            function->set_llvm_data(gen, func_ptr);
        } else {
            auto& overridable_info = llvm_data->second;
            // indicate to interface that this function has been overridden
            // so default implementation must not be generated (very important)
            overridable_info.overridden = true;
            // clean the function (any default implementation from interface may be there)
            const auto func = overridable_info.func_pointer;
            gen.cleanFunctionEntryBlock(func);
            function->set_llvm_data(gen, func);
        }
    }
}

#endif

ASTNode *VariablesContainer::child_member_or_inherited_struct(const chem::string_view& name) {
    auto direct_var = direct_variable(name);
    if(direct_var) return direct_var;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->get_direct_linked_struct();
        if(struct_def && struct_def->name_view().data() == name) {
            return struct_def;
        }
    }
    return nullptr;
}

BaseDefMember *VariablesContainer::inherited_member(const chem::string_view& name) {
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->get_direct_linked_struct();
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
    // TODO: using default target data
    // this function is being used by other functions
    auto created = create_target_data();
    for(const auto var : variables()) {
        if(member == nullptr || var->byte_size(created) > member->byte_size(created)) {
            member = var;
        }
    }
    return member;
}

void VariablesContainer::ensure_inherited_visibility(ASTDiagnoser& diagnoser, AccessSpecifier at_least_spec) {
    for(auto& inh : inherited) {
        const auto linked = inh.type->get_direct_linked_node();
        if(linked) {
            if(linked->specifier() < at_least_spec) {
                diagnoser.error(inh.type.encoded_location()) << "must refer to a node with at least '" << to_string(at_least_spec) << "' access specifier";
            }
        }
    }
}

uint64_t VariablesContainer::total_byte_size(TargetData& target) {
    size_t offset = 0;
    size_t maxAlignment = 1;
    for (const auto member : variables()) {
        // Update max alignment
        const auto member_alignment = (size_t) member->known_type()->type_alignment(target);
        maxAlignment = std::max(maxAlignment, member_alignment);
        // Align the current offset
        size_t padding = (member_alignment - (offset % member_alignment)) % member_alignment;
        offset += padding;

        // Add the size of the member
        offset += member->byte_size(target);
    }
    // Align the total size to the largest alignment
    size_t totalPadding = (maxAlignment - (offset % maxAlignment)) % maxAlignment;
    offset += totalPadding;
    return offset;
}

uint64_t VariablesContainer::largest_member_byte_size(TargetData& target) {
    uint64_t size = 0;
    uint64_t previous;
    for (const auto mem: variables()) {
        previous = mem->byte_size(target);
        if (previous > size) {
            size = previous;
        }
    }
    return size;
}

unsigned int MembersContainer::init_values_req_size() {
    unsigned int i = 0;
    for(auto& inherit : inherited) {
        auto direct = inherit.type->get_direct_linked_struct();
        if(direct && !direct->variables().empty()) {
            i++;
        }
    }
    for(auto& var : variables()) {
        if(var->default_value() == nullptr) {
            i++;
        }
    }
    return i;
}

bool MembersContainer::getAllMembersDefaultInitialized() {

    if(default_initialized.has_value()) {
        return default_initialized.value();
    }

    // state variable
    bool final_def_initialized = true;
    bool variables_initialized = true;

    // check all variables have been default initialized
    for(const auto var : variables()) {
        if(var->default_value() != nullptr) {
            continue;
        }
        variables_initialized = false;
        const auto container = var->known_type()->get_members_container();
        if(container) {
            if(container == this) {
                // self composition (recursion)
                continue;
            }
            if(!container->getAllMembersDefaultInitialized()) {
                final_def_initialized = false;
            }
        } else {
            final_def_initialized = false;
        }
    }

    // check if all inherited members are default initialized
    for(auto& inh : inherited) {
        const auto container = inh.type->get_members_container();
        if(container == this) {
            // self inheritance (recursion)
            continue;
        }
        const auto initialized = container->getAllMembersDefaultInitialized();
        if(!initialized) {
            final_def_initialized = false;
        }
    }

    // check if default constructor exists
    const auto cons = default_constructor_func();
    if(cons) {
        setAllMembersDefaultInitialized(true);
        return true;
    }

    setAllMembersDefaultInitialized(final_def_initialized);
    return true;

}

void MembersContainer::take_members_from_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& from_nodes) {
    for(const auto node : from_nodes) {
        switch(node->kind()) {
            case ASTNodeKind::StructMember:
            case ASTNodeKind::UnnamedStruct:
            case ASTNodeKind::UnnamedUnion:
            case ASTNodeKind::VariantMember:
                if(!insert_variable(node->as_base_def_member_unsafe())) {
                    linker.error(node) << "couldn't insert member because '" << node->as_base_def_member_unsafe()->name << "' already exists";
                }
                break;
            case ASTNodeKind::IfStmt: {
                const auto stmt = node->as_if_stmt_unsafe();
                const auto scope = stmt->get_evaluated_scope_by_linking(linker);
                if(scope) {
                    take_members_from_parsed_nodes(linker, scope->nodes);
                }
                break;
            }
            case ASTNodeKind::UnsafeBlock:
                // TODO make all nodes unsafe
                take_members_from_parsed_nodes(linker, node->as_unsafe_block_unsafe()->scope.nodes);
                break;
            case ASTNodeKind::FunctionDecl:
                insert_multi_func(*linker.ast_allocator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                // TODO multi function node doesn't support generic functions
                insert_func(node->as_gen_func_decl_unsafe());
                break;
            default:
                break;
        }
    }
}

//void MembersContainer::declare_and_link_no_scope(SymbolResolver &linker) {
//    for(auto& inherits : inherited) {
//        const auto def = inherits.type->get_members_container();
//        if(def) {
//            declare_inherited_members(def, linker);
//        }
//    }
//    // this will only declare aliases
//    declare_parsed_nodes(linker);
//    // declare all the variables manually
//    for (const auto var : variables()) {
//        if(var->name.empty()) {
//#ifdef DEBUG
//            switch(var->kind()) {
//                case ASTNodeKind::UnnamedStruct:
//                case ASTNodeKind::UnnamedUnion:
//                    break;
//                default:
//                    CHEM_THROW_RUNTIME("why does this variable has empty name");
//            }
//#endif
//            continue;
//        } else {
//            linker.declare(var->name, var);
//        }
//    }
//    // declare all the functions
//    TopLevelDeclSymDeclare declarer(linker);
//
//    for(auto& func : functions()) {
//        declarer.visit(func);
//    }
//    for (auto& func: functions()) {
//        func->declare_and_link(linker, (ASTNode*&) func);
//    }
//}
//
//void MembersContainer::declare_and_link(SymbolResolver &linker) {
//    linker.scope_start();
//    declare_and_link_no_scope(linker);
//    linker.scope_end();
//}

void MembersContainer::register_use_to_inherited_interfaces(ExtendableMembersContainerNode* definition) {
    for(auto& inherits : inherited) {
        const auto interface = inherits.type->get_direct_linked_interface();
        if(interface) {
            interface->register_use(definition);
            interface->register_use_to_inherited_interfaces(definition);
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
            if(func2) return func2;
        }
    }
    return nullptr;
}

FunctionDeclaration *MembersContainer::direct_child_function(const chem::string_view& name) {
    auto func = indexes.find(name);
    return func != indexes.end() ? func->second->as_function() : nullptr;
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

FunctionDeclaration* MembersContainer::get_first_constructor() {
    for(const auto function : non_gen_range()) {
        if(function->is_constructor_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::destructor_func() {
    for (const auto function : non_gen_range()) {
        if(function->is_delete_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::copy_func() {
    for (const auto function : non_gen_range()) {
        if(function->is_copy_fn()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::default_constructor_func() {
    for(const auto function : non_gen_range()) {
        if(function->is_constructor_fn() && function->params.empty()) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::constructor_func(std::vector<Value*>& forArgs) {
    for (const auto function : non_gen_range()) {
        if(function->is_constructor_fn() && function->satisfy_args(forArgs)) {
            return function;
        }
    }
    return nullptr;
}

FunctionDeclaration* MembersContainer::implicit_constructor_func(Value* value) {
    for (const auto & function : non_gen_range()) {
        if(function->is_implicit() && function->params.size() == 1 && function->params[0]->type->satisfies(value, false)) {
            return function;
        }
    }
    return nullptr;
}

bool all_variables_type_require(VariablesContainer& container, bool(*requirement)(BaseType*, Value*), bool variant_container) {
    for(const auto& inh : container.inherited) {
        if(!requirement(inh.type, nullptr)) {
            return false;
        }
    }
    if(variant_container) {
        for(const auto var : container.variables()) {
            const auto mem = var->as_variant_member_unsafe();
            for(auto& val : mem->values) {
                if(!requirement(val.second->type, val.second->def_value)) {
                    return false;
                }
            }
        }
        return true;
    } else {
        for(const auto var : container.variables()) {
            switch(var->kind()) {
                case ASTNodeKind::UnnamedStruct:
                    if(!all_variables_type_require(*var->as_unnamed_struct_unsafe(), requirement, false)){
                        return false;
                    }
                    break;
                case ASTNodeKind::UnnamedUnion:
                    if(!all_variables_type_require(*var->as_unnamed_union_unsafe(), requirement, false)){
                        return false;
                    }
                    break;
                case ASTNodeKind::StructMember:
                    if(!requirement(var->as_struct_member_unsafe()->type, var->as_struct_member_unsafe()->defValue)) {
                        return false;
                    }
                    break;
                default:
                    break;
            }
        }
        return true;
    }
}

bool variables_type_require(VariablesContainer& container, bool(*requirement)(BaseType*), bool variant_container) {
    for(const auto& inh : container.inherited) {
        if(requirement(inh.type->canonical())) {
            return true;
        }
    }
    if(variant_container) {
        for(const auto var : container.variables()) {
            const auto mem = var->as_variant_member_unsafe();
            for(auto& val : mem->values) {
                if(requirement(val.second->type->canonical())) {
                    return true;
                }
            }
        }
        return false;
    } else {
        for(const auto var : container.variables()) {
            switch(var->kind()) {
                case ASTNodeKind::UnnamedStruct:
                    if(variables_type_require(*var->as_unnamed_struct_unsafe(), requirement, false)){
                        return true;
                    }
                    break;
                case ASTNodeKind::UnnamedUnion:
                    if(variables_type_require(*var->as_unnamed_union_unsafe(), requirement, false)){
                        return true;
                    }
                    break;
                case ASTNodeKind::StructMember:
                    if(requirement(var->as_struct_member_unsafe()->type->canonical())) {
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

inline bool one_member_type_requires(MembersContainer& container, bool(*requirement)(BaseType*)) {
    return variables_type_require(container, requirement, ASTNode::isVariantDecl(container.kind()));
}

inline bool all_members_type_require(MembersContainer& container, bool(*requirement)(BaseType*, Value*)) {
    return all_variables_type_require(container, requirement, ASTNode::isVariantDecl(container.kind()));
}

bool MembersContainer::all_members_def_constructible() {
    return all_members_type_require(*this, [](BaseType* type, Value* defValue)-> bool {
        const auto container = type->get_members_container();
        if(container) {
            return container->default_constructor_func() != nullptr;
        } else {
            // non container value must have default value
            // for constructor to be generated
            return defValue != nullptr;
        }
    });
}

bool MembersContainer::any_member_has_destructor() {
    return one_member_type_requires(*this, [](BaseType* type)-> bool {
        if(type->kind() == BaseTypeKind::CapturingFunction) {
            return type->as_capturing_func_type_unsafe()->instance_type->get_destructor() != nullptr;
        } else {
            return type->get_destructor() != nullptr;
        }
    });
}

bool MembersContainer::any_member_has_copy_func() {
    return one_member_type_requires(*this, [](BaseType* type)-> bool {
        return type->get_copy_fn() != nullptr;
    });
}

void MembersContainer::insert_func(FunctionDeclaration* decl) {
    functions_container.emplace_back(decl);
    indexes[decl->name_view()] = decl;
}

void MembersContainer::insert_func(GenericFuncDecl* decl) {
    functions_container.emplace_back(decl);
    indexes[decl->master_impl->name_view()] = decl;
}

void MembersContainer::insert_functions(const std::initializer_list<FunctionDeclaration*>& decls) {
    for(const auto d : decls) {
        insert_func(d);
    }
}

FunctionDeclaration* MembersContainer::create_def_constructor(ASTAllocator& allocator, const chem::string_view& parent_name, ASTNode* returnNode) {
    const auto loc = encoded_location();
    const auto returnType = new (allocator.allocate<LinkedType>()) LinkedType(returnNode);
    const auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("make"), {returnType, loc}, false, this, loc);
    decl->body.emplace(Scope{nullptr, loc});
    decl->set_is_generated_fn(true);
    decl->set_constructor_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_destructor(ASTAllocator& allocator, ASTNode* returnNode) {
    const auto loc = encoded_location();
    const auto returnType = TypeLoc(new (allocator.allocate<VoidType>()) VoidType(), loc);
    const auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("delete"), returnType, false, this, loc);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", { new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(returnNode), true), loc }, 0, nullptr, true, decl, loc));
    decl->body.emplace(Scope{nullptr, loc});
    decl->set_is_generated_fn(true);
    decl->set_delete_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_copy_fn(ASTAllocator& allocator, ASTNode* returnNode) {
    const auto loc = encoded_location();
    const auto returnType = TypeLoc(new (allocator.allocate<VoidType>()) VoidType(), loc);
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(ZERO_LOC_ID("copy"), returnType, false, this, loc);
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", { new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(returnNode), true), loc }, 0, nullptr, true, decl, loc));
    decl->params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("other", { new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<LinkedType>()) LinkedType(returnNode), true), loc }, 1, nullptr, true, decl, loc));
    decl->body.emplace(Scope{nullptr, loc});
    decl->set_is_generated_fn(true);
    decl->set_copy_fn(true);
    insert_func(decl);
    return decl;
}

FunctionDeclaration* MembersContainer::create_def_constructor_checking(ASTAllocator& allocator, ASTDiagnoser& diagnoser, const chem::string_view& container_name, ASTNode* returnNode) {
    auto delFunc = direct_child_function("make");
    if(delFunc) {
        diagnoser.warn("default constructor is created by name 'make', a function by name 'make' already exists, please create a manual constructor to avoid this", (AnnotableNode*) delFunc);
        return nullptr;
    }
    return create_def_constructor(allocator, container_name, returnNode);
}

FunctionDeclaration* MembersContainer::create_def_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
    auto delFunc = direct_child_function("delete");
    if(delFunc) {
        diagnoser.warn("default destructor is created by name 'delete', a function by name 'delete' already exists, please create a manual destructor to avoid this", (AnnotableNode*) delFunc);
        return nullptr;
    }
    return create_destructor(allocator, returnNode);
}

FunctionDeclaration* MembersContainer::create_def_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
    auto copyFn = direct_child_function("copy");
    if(copyFn) {
        diagnoser.warn("default copy function is created by name 'copy', a function by name 'copy' already exists, please create a manual copy function to avoid this", (AnnotableNode*) copyFn);
        return nullptr;
    }
    return create_copy_fn(allocator, returnNode);
}

bool MembersContainer::insert_multi_func(ASTAllocator& astAllocator, FunctionDeclaration* decl) {
    auto found = indexes.find(decl->name_view());
    if(found == indexes.end()) {
        insert_func(decl);
    } else {
        auto result = handle_name_overload_function(astAllocator, found->second, decl);
        if(result.specifier_mismatch || !result.duplicates.empty()) {
            // TODO handle errors for duplicates and specifier mismatch
            return false;
        } else if(result.new_multi_func_node) {
            // TODO -1 is being stored as index
            indexes[decl->name_view()] = result.new_multi_func_node;
        }
        functions_container.emplace_back(decl);
    }
    return true;
}

bool MembersContainer::contains_func(const chem::string_view& name) {
    return indexes.find(name) != indexes.end();
}

BaseType* MembersContainer::create_linked_type(const chem::string_view& name, ASTAllocator& allocator) {
    const auto linked_type = new (allocator.allocate<LinkedType>()) LinkedType(this);
    return linked_type;
}

bool MembersContainer::extends_node(ASTNode* other) {
    if(inherited.empty()) {
        return false;
    }
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
            if(!inherited_node) {
                continue;
            }
            if(inherited_node == other) {
                return true;
            }
            const auto container = inherited_node->get_members_container();
            if(container && container->extends_node(other)) {
                return true;
            }
        }
    }
    return false;
}

std::pair<long, BaseType*> VariablesContainer::variable_type_index(const chem::string_view& varName, bool consider_inherited_structs) {
    long parents_size = 0;
    for(auto& inherits : inherited) {
        const auto struct_def = inherits.type->linked_node()->as_struct_def();
        if(struct_def) {
            if(consider_inherited_structs && struct_def->name_view() == varName) {
                // user wants the struct
                return { parents_size, inherits.type };
            }
            parents_size += 1;
        }
    }
    auto found = indexes.find(varName);
    if(found == indexes.end()) {
        return { -1, nullptr };
    } else {
        return { direct_mem_index(found->second->as_base_def_member_unsafe()) + parents_size, found->second->known_type() };
    }
}

long VariablesContainer::direct_mem_index(BaseDefMember* member) {
    // calculating index of child by looping over direct variables
    long i = 0;
    for(const auto var : variables()) {
        if(var == member) {
            return i;
        }
        i++;
    }
    return -1;
}

long VariablesContainer::direct_child_index(const chem::string_view& varName) {
    const auto c = get_child(varName);
    if(c == nullptr) return -1;
    return direct_mem_index(c->as_base_def_member_unsafe());
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

void VariablesContainer::adopt(MembersContainer* definition) {
    for(auto& inherits : definition->inherited) {
        // adopt inherited container
        const auto container = inherits.type->get_members_container();
        if(container) adopt(container);
    }
    for(const auto node : definition->evaluated_nodes()) {
        switch(node->kind()) {
            case ASTNodeKind::FunctionDecl:
                // will not overwrite any existing function with same name
                indexes.try_emplace(node->as_function_unsafe()->name_view(), node);
                break;
            case ASTNodeKind::GenericFuncDecl:
                // will not overwrite any existing function with same name
                indexes.try_emplace(node->as_gen_func_decl_unsafe()->master_impl->name_view(), node);
                break;
            default:
                break;
        }
    }
}

bool VariablesContainer::build_path_to_child(std::vector<int>& path, const chem::string_view& child_name) {
    const auto child_ind = direct_child_index(child_name);
    if(child_ind != -1) {
        path.emplace_back(child_ind);
        return true;
    }
    auto inherit_index = 0;
    for(auto& inherits : inherited) {
        const auto linked_struct = inherits.type->get_direct_linked_struct();
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

void VariablesContainer::take_variables_from_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& from_nodes) {
    for(const auto node : from_nodes) {
        switch(node->kind()) {
            case ASTNodeKind::StructMember:
            case ASTNodeKind::UnnamedStruct:
            case ASTNodeKind::UnnamedUnion:
            case ASTNodeKind::VariantMember:{
                const auto member = node->as_base_def_member_unsafe();
                if(!insert_variable(member)) {
                    linker.error(member) << "couldn't insert because a member with name '" << member->name << "' already exists";
                }
                break;
            }
            case ASTNodeKind::IfStmt: {
                const auto stmt = node->as_if_stmt_unsafe();
                stmt->link_conditions(linker);
                const auto scope = stmt->get_evaluated_scope_by_linking(linker);
                if(scope) {
                    take_variables_from_parsed_nodes(linker, scope->nodes);
                }
                break;
            }
            default:
                break;
        }
    }
}

void VariablesContainer::declare_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::AliasStmt:{
                TopLevelDeclSymDeclare declarer(linker);
                declarer.visit(node);
                break;
            }
            case ASTNodeKind::IfStmt: {
                const auto stmt = node->as_if_stmt_unsafe();
                const auto scope = stmt->get_evaluated_scope_by_linking(linker);
                if(scope) {
                    declare_parsed_nodes(linker, scope->nodes);
                }
                break;
            }
            default:
                break;
        }
    }
}

chem::string_view InheritedType::ref_type_name() {
    if(type->kind() == BaseTypeKind::Generic) {
        return ((GenericType*) type.getType())->referenced->linked_name();
    } else if(type->kind() == BaseTypeKind::Linked) {
        return ((LinkedType*) type.getType())->linked_name();
    }
#ifdef DEBUG
    CHEM_THROW_RUNTIME("unable to retrieve referenced type name from type");
#else
    std::cerr << "unable to retrieve referenced type name from type " + type->representation() << std::endl;
#endif
    return "";
}

InheritedType InheritedType::copy(ASTAllocator& allocator) const {
    return InheritedType(type.copy(allocator), specifier);
}