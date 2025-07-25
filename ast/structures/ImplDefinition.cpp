// Copyright (c) Chemical Language Foundation 2025.

#include "ImplDefinition.h"
#include "StructMember.h"
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
            decl->set_llvm_data(gen, found->second);
            decl->code_gen_override(gen, found->second);
        } else {
            decl->code_gen_override_declare(gen, overridden.second);
            decl->code_gen_override(gen, overridden.second->llvm_func(gen));
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

void InterfaceDefinition::register_impl(ImplDefinition* definition) {
    const auto struct_linked = definition->struct_type ? definition->struct_type->linked_struct_def() : nullptr;
    if(struct_linked) {
        register_use(struct_linked);
        register_use_to_inherited_interfaces(struct_linked);
    }
    attrs.has_implementation = true;
}