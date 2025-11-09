// Copyright (c) Chemical Language Foundation 2025.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "std/except.h"

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
            const auto func_pointer = found->second.func_pointer;
            decl->set_llvm_data(gen, func_pointer);
            decl->code_gen_override(gen, func_pointer);
        } else {
            const auto func_pointer = overridden.second->llvm_func(gen);
            decl->set_llvm_data(gen, func_pointer);
            decl->code_gen_override(gen, func_pointer);
        }
    } else {
        gen.error("failed to override function in impl because not found", (AnnotableNode*) decl);
    }
}

void ImplDefinition::code_gen_function_primitive(Codegen& gen, FunctionDeclaration* decl, InterfaceDefinition* linked) {
    decl->code_gen_body(gen);
}

void ImplDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    const auto linked = interface_type->linked_node()->as_interface_def();
    const auto struct_def = struct_type ? struct_type->linked_struct_def() : nullptr;
    code_gen_function(gen, decl, linked, struct_def);
}

void ImplDefinition::code_gen_declare(Codegen &gen) {
    // const auto linked = interface_type->linked_node()->as_interface_def();
    if(struct_type == nullptr) return;
    // struct type is given, but probably primitive
    const auto struct_def = struct_type->get_direct_linked_struct();
    if(struct_def) {
        // nothing to do here
    } else {
        for (auto& function: instantiated_functions()) {
            function->code_gen_declare_normal(gen);
        }
    }
}

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->linked_node()->as_interface_def();
    const auto struct_def = struct_type ? struct_type->get_direct_linked_struct() : nullptr;
    // struct type is given, but probably primitive
    if(struct_type != nullptr && struct_def == nullptr) {
        for (auto& function: instantiated_functions()) {
            code_gen_function_primitive(gen, function, linked);
        }
    } else {
        for (auto& function: instantiated_functions()) {
            code_gen_function(gen, function, linked, struct_def);
        }
        if (linked && struct_def) {
            linked->llvm_global_vtable(gen, struct_def);
        }
    }
}

#endif

uint64_t ImplDefinition::byte_size(bool is64Bit) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("ImplDefinition::byte_size byte_size on impl definition");
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