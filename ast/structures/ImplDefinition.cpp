// Copyright (c) Chemical Language Foundation 2025.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void ImplDefinition::code_gen_function(Codegen& gen, FunctionDeclaration* decl, InterfaceDefinition* linked, StructDefinition* struct_def) {
    auto overridden = linked->get_func_with_signature(decl);
    if (overridden.first) {
        const auto interface_def = overridden.first->as_interface_def();
        if(!interface_def) {
            gen.error("failed to override function in impl, because function not present in an interface above", (AnnotableNode*) decl);
            return;
        }
        if(struct_def) {
            const auto& use = interface_def->users[struct_def];
            auto found = use.find(overridden.second);
            if(found == use.end()) {
                gen.error("failed to override function in impl because declaration not found", (AnnotableNode*) decl);
                return;
            }
            decl->set_llvm_data(found->second);
            decl->code_gen_override(gen, found->second);
        } else {
            decl->code_gen_override_declare(gen, overridden.second);
            decl->code_gen_override(gen, overridden.second->llvm_func());
        }
    } else {
        gen.error("failed to override function in impl because not found", (AnnotableNode*) decl);
    }
}

void ImplDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    const auto linked = interface_type->linked_node()->as_interface_def();
    const auto struct_def = struct_type ? struct_type->linked_struct_def() : nullptr;
    code_gen_function(gen, decl, linked, struct_def);
}

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->linked_node()->as_interface_def();
    const auto struct_def = struct_type ? struct_type->linked_struct_def() : nullptr;
    for (auto& function: instantiated_functions()) {
        code_gen_function(gen, function, linked, struct_def);
    }
    if(linked && struct_def) {
        linked->llvm_global_vtable(gen, struct_def);
    }
}

#endif

uint64_t ImplDefinition::byte_size(bool is64Bit) {
#ifdef DEBUG
    throw std::runtime_error("ImplDefinition::byte_size byte_size on impl definition");
#endif
    return 0;
}

void ImplDefinition::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    take_members_from_parsed_nodes(linker);
    interface_type->link(linker);
    if(struct_type) {
        struct_type->link(linker);
    }
    const auto linked = interface_type->linked_node();
    if(linked) {
        const auto interface_def = linked->as_interface_def();
        if(interface_def) {
            if(interface_def->is_static() && interface_def->has_implementation()) {
                linker.error("static interface must have only a single implementation", interface_type);
            }
            interface_def->register_impl(this);
        } else {
            linker.error("expected type to be an interface", interface_type);
        }
    }
}

void ImplDefinition::link_signature_no_scope(SymbolResolver &linker) {
    const auto& interface_name = interface_type->linked_name();
    const auto linked_node = interface_type->linked_node();
    if(!linked_node) {
        return;
    }
    const auto linked = linked_node->as_interface_def();
    if(!linked) {
        linker.error(interface_type) << "couldn't find interface by name " << interface_name << " for implementation";
        return;
    }
    for(auto& func : master_functions()) {
        if(!func->is_override()) {
            func->set_override(true);
        }
    }
    linker.scope_start();
    const auto struct_linked = struct_type ? struct_type->linked_struct_def() : nullptr;
    MembersContainer::link_signature_no_scope(linker);
    linker.scope_end();
    if(struct_linked) {
        // adding all methods of this implementation to struct
        struct_linked->adopt(this);
    }
}

void ImplDefinition::link_signature(SymbolResolver& linker) {
    linker.scope_start();
    link_signature_no_scope(linker);
    linker.scope_end();
}

void ImplDefinition::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    const auto linked_node = interface_type->linked_node();
    if(!linked_node) {
        return;
    }
    const auto linked = linked_node->as_interface_def();
    if(!linked) {
        return;
    }
    linker.scope_start();
    const auto struct_linked = struct_type ? struct_type->linked_struct_def() : nullptr;
    const auto overrides_interface = struct_linked && struct_linked->does_override(linked);
    if(!overrides_interface) {
        for (const auto func: linked->functions()) {
            switch(func->kind()) {
                case ASTNodeKind::FunctionDecl:
                    linker.declare(func->as_function_unsafe()->name_view(), func);
                    break;
                case ASTNodeKind::GenericFuncDecl:
                    linker.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                    break;
                default:
                    break;
            }
        }
    }
    // redeclare everything inside struct
    if(struct_linked) {
        struct_linked->redeclare_inherited_members(linker);
        struct_linked->redeclare_variables_and_functions(linker);
    }
    MembersContainer::declare_and_link_no_scope(linker);
    linker.scope_end();
}

void InterfaceDefinition::register_impl(ImplDefinition* definition) {
    const auto struct_linked = definition->struct_type ? definition->struct_type->linked_struct_def() : nullptr;
    if(struct_linked) {
        register_use(struct_linked);
        register_use_to_inherited_interfaces(struct_linked);
    }
    attrs.has_implementation = true;
}