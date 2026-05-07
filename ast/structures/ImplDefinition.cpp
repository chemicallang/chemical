// Copyright (c) Chemical Language Foundation 2025.

#include "ImplDefinition.h"

#include "GenericInterfaceDecl.h"
#include "StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/frontend/AnnotationController.h"
#include "std/except.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void ImplDefinition::code_gen_function(Codegen& gen, FunctionDeclaration* decl, InterfaceDefinition* linked, ExtendableMembersContainerNode* struct_def) {
    // TODO: this is not the best way to get the base function
    auto overridden = linked->get_func_with_signature(decl);
    if (overridden.first) {
        const auto interface_def = overridden.first->as_interface_def();
        if(!interface_def) {
            gen.error("failed to override function in impl, because function not present in an interface above", (AnnotableNode*) decl);
            return;
        }
        if(struct_def && !interface_def->is_static()) {
            // lets get the function pointer for this base function
            const auto key = TraitImplFuncMapKey { .interface = interface_def, .for_ = struct_def, .func = overridden.second };
            auto found = gen.trait_impl_func_map.find(key);
            if (found == gen.trait_impl_func_map.end()) {
                gen.error("failed to find declared function pointer when implementing body", decl);
                return;
            }
            const auto func_pointer = found->second;
            decl->set_llvm_data(gen, func_pointer);
            decl->code_gen_override(gen, func_pointer);
        } else {
            const auto func_pointer = overridden.second->llvm_func(gen);
            gen.cleanFunctionEntryBlock(func_pointer);
            decl->set_llvm_data(gen, func_pointer);
            decl->code_gen_override(gen, func_pointer);
        }
    } else {
        gen.error("failed to override function in impl because not found", (AnnotableNode*) decl);
    }
}

void ImplDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    const auto linked = interface_type->get_direct_linked_interface();
    const auto canonical_node = struct_type ? struct_type->get_direct_linked_canonical_node() : nullptr;
    const auto container = canonical_node ? canonical_node->as_extendable_member_container() : nullptr;
    code_gen_function(gen, decl, linked, container);
}

void ImplDefinition::code_gen_bodies(Codegen& gen, InterfaceDefinition* interface, ExtendableMembersContainerNode* user) {
    for (const auto func : interface->instantiated_functions()) {
        // TODO: find a better way to get the implementation function, currently using name
        // why can't we use the override_map (because that maps functions from the master interface)
        // however this interface is an instantiated interface (not the template)
        const auto decl = direct_child_function(func->name_view());
        if (decl == nullptr && !func->body.has_value()) {
            gen.error("couldn't find implementation function when implementing body", func);
            gen.warn("failed to implement impl", this);
            continue;
        }
        if (interface->is_static()) {
            const auto func_pointer = func->llvm_func(gen);
            // must update the linkage, so strong implementation overrides the weak implementation at link time
            const auto final_specifier = is_linkage_public(interface->specifier()) || is_linkage_public(specifier()) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
            func_pointer->setLinkage(final_specifier);
            // override
            gen.cleanFunctionEntryBlock(func_pointer);
            if (decl == nullptr) {
                // we have a default implementation available
                func->code_gen_override(gen, func_pointer);
            } else {
                decl->set_llvm_data(gen, func_pointer);
                decl->code_gen_override(gen, func_pointer);
            }
        } else {
            // lets get the function pointer for this base function
            const auto key = TraitImplFuncMapKey { .interface = interface, .for_ = user, .func = func };
            auto found = gen.trait_impl_func_map.find(key);
            if (found == gen.trait_impl_func_map.end()) {
                gen.error("failed to find declared function pointer when implementing body", func);
                gen.warn("failed to implement impl", this);
                continue;
            }
            const auto func_pointer = found->second;
            if (decl == nullptr) {
                func->code_gen_override(gen, func_pointer);
            } else {
                decl->set_llvm_data(gen, func_pointer);
                decl->code_gen_override(gen, func_pointer);
            }
        }
    }
    // going over inherited interfaces to implement the given interface
    for (auto& inh : interface->inherited) {
        const auto can = inh.type->get_direct_linked_interface();
        if (can) {
            code_gen_bodies(gen, can, user);
        }
    }
}

void ImplDefinition::strengthen_static_declare(Codegen& gen, InterfaceDefinition* interface, ExtendableMembersContainerNode* node) {
    const auto prev_user = interface->active_user;
    interface->active_user = node;
    const auto final_specifier = is_linkage_public(interface->specifier()) || is_linkage_public(specifier()) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
    for (const auto func: interface->instantiated_functions()) {
        auto func_ptr = func->get_llvm_data(gen);
        if (func_ptr == nullptr) {
            gen.error("couldn't get function pointer when implementing static interface during declaration", func);
            gen.warn("couldn't implement this impl", this);
            continue;
        }
        // TODO: find a better way to get the implementation function, currently using name
        // why can't we use the override_map (because that maps functions from the master interface)
        // however this interface is an instantiated interface (not the template)
        const auto decl = direct_child_function(func->name_view());
        if (decl != nullptr) {
            decl->set_llvm_data(gen, func_ptr);
        }
        // clean the function entry block, set new linkage
        gen.cleanFunctionEntryBlock(func_ptr);
        func_ptr->setLinkage(final_specifier);
    }
    // going over inherited interfaces and calling the same function
    for (auto& inh : interface->inherited) {
        const auto can = inh.type->get_direct_linked_interface();
        if (can) {
            strengthen_static_declare(gen, can, node);
        }
    }
    interface->active_user = prev_user;
}

void ImplDefinition::code_gen_declare(Codegen &gen) {
    if(struct_type == nullptr) return;
    const auto linked = interface_type->get_direct_linked_interface();
    // struct type is given, but probably primitive
    const auto canonical_node = struct_type->get_direct_linked_canonical_node();
    const auto struct_def = canonical_node ? canonical_node->as_extendable_member_container() : nullptr;
    if(struct_def) {
        // impl came before interface (but thats not the problem)
        // or impl exists in this module and interface exists in a non-imported module
        // in this case we need to declare the methods
        linked->code_gen_declare_for_user(gen, struct_def);
    } else {
        if(linked->is_static()) {
            strengthen_static_declare(gen, linked, nullptr);
        } else {
            for (const auto function: instantiated_functions()) {
                function->code_gen_declare_normal(gen);
            }
        }
    }
}

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->get_direct_linked_interface();
    const auto canonical_node = struct_type ? struct_type->get_direct_linked_canonical_node() : nullptr;
    const auto struct_def = canonical_node ? canonical_node->as_extendable_member_container() : nullptr;
    if(struct_type != nullptr && struct_def == nullptr) {
        // struct type is given, but no definition, probably primitive
        for (const auto function: instantiated_functions()) {
            function->code_gen_body(gen);
        }
        if(linked->generates_vtable()) {
            linked->create_global_vtable(gen, this, struct_type, false);
        }
    } else {
        // struct_def nullable is acceptable to this function
        code_gen_bodies(gen, linked, struct_def);
        if (linked->generates_vtable() && struct_def) {
            // this only creates the vtable (if it doesn't already exist)
            linked->llvm_global_vtable(gen, struct_def);
        }
    }
}

void ImplDefinition::code_gen_external_declare(Codegen &gen) {
    // const auto linked = interface_type->linked_node()->as_interface_def();
    if(struct_type == nullptr) return;
    const auto interface = interface_type->get_direct_linked_interface();
    // struct type is given, but probably primitive
    const auto canonical_node = struct_type->get_direct_linked_canonical_node();
    const auto user_def = canonical_node ? canonical_node->as_extendable_member_container() : nullptr;
    if(user_def) {
        // suppose user imported a module that contains this impl, but not the interface
        // we must declare the methods and vtable, so user can call them
        // but if interface & impl are in the same module, this leads to multiple declarations (because code_gen_external_declare called on both of them)
        // currently we are letting this happen
        interface->code_gen_external_declare_for_user(gen, user_def);
        if(interface->generates_vtable()) {
            // declare the vtable if it hasn't been declared (because maybe interface is not present in same module, so we must declare)
            // always declare, creation is handled in code_gen
            // it will never be a new user, because this impl is present in external module
            interface->create_global_vtable(gen, user_def, true);
        }
    } else {
        if(!is_linkage_public(specifier())) {
            return;
        }
        for (const auto function: instantiated_functions()) {
            function->code_gen_external_declare(gen, AccessSpecifier::Public);
        }
        // just declare the global vtable for this primitive impl Again.
        if(interface->generates_vtable()) {
            interface->create_global_vtable(gen, this, struct_type, true);
        }
    }
}

#endif

uint64_t ImplDefinition::byte_size(TargetData& target) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("ImplDefinition::byte_size byte_size on impl definition");
#endif
    return 0;
}

void InterfaceDefinition::register_impl(ImplDefinition* definition) {
    attrs.has_implementation = true;
    if (definition->struct_type == nullptr) return;
    const auto linked = definition->struct_type->get_direct_linked_canonical_node();
    if (linked) {
        switch (linked->kind()) {
            case ASTNodeKind::StructDecl:
            case ASTNodeKind::UnionDecl:
            case ASTNodeKind::VariantDecl:
                register_use(linked->as_extendable_members_container_unsafe());
                register_use_to_inherited_interfaces(linked->as_extendable_members_container_unsafe());
                return;
            default:
                return;
        }
    }
}

void ImplDefinition::index_implementations(ASTDiagnoser& diagnoser, InterfaceDefinition* non_master_interface) {
    // why use master (template in generic definition) ?
    // we must index against the function present in the master (template) interface
    // because we use that to lookup the implementation
    const auto interface = non_master_interface->generic_parent != nullptr ? non_master_interface->generic_parent->as_gen_interface_decl_unsafe()->master_impl : non_master_interface;
    for (const auto node : evaluated_nodes()) {
        FunctionDeclaration* func;
        if (node->kind() == ASTNodeKind::FunctionDecl) {
            func = node->as_function_unsafe();
        } else if (node->kind() == ASTNodeKind::GenericFuncDecl) {
            func = node->as_gen_func_decl_unsafe()->master_impl;
        } else {
            continue;
        }
        const auto child = interface->child(func->name_view());
        if (child == nullptr) {
            diagnoser.error(func) << "couldn't find base function to override";
            continue;
        }
        override_map[child] = node;
    }
}