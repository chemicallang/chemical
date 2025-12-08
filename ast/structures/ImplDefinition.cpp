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
        if(struct_def && !interface_def->is_static()) {
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
    const auto struct_def = struct_type ? struct_type->get_direct_linked_struct() : nullptr;
    code_gen_function(gen, decl, linked, struct_def);
}

void ImplDefinition::code_gen_declare(Codegen &gen) {
    if(struct_type == nullptr) return;
    const auto linked = interface_type->get_direct_linked_interface();
    // struct type is given, but probably primitive
    const auto struct_def = struct_type->get_direct_linked_struct();
    if(struct_def) {
        for (auto& function : instantiated_functions()) {
            auto overridden = linked->get_func_with_signature(function);
            if(overridden.second == nullptr) {
                gen.error("couldn't get base function when determining function pointer", (AnnotableNode*) function);
            } else {
                const auto func = overridden.second;
                const auto func_ptr = func->known_func(gen);
                if(func_ptr == nullptr) {
                    // impl probably came before the interface, so now we declare it before interface
                    if(func->has_self_param()) {
                        func->code_gen_declare(gen, linked);
                    }
                    auto& use = linked->users[struct_def];
                    const auto new_func_ptr = func->get_llvm_data(gen);
                    use[func] = { new_func_ptr, false };
                    function->set_llvm_data(gen, new_func_ptr);
                } else {
                    function->set_llvm_data(gen, func_ptr);
                }
            }
        }
    } else {
        if(linked->is_static()) {
            for (const auto function: instantiated_functions()) {
                // need to use the function pointer of the interface
                const auto base_func = linked->direct_or_inherited_function(function->name_view());
                if(base_func == nullptr) {
                    gen.error("couldn't override function", function);
                    function->code_gen_declare_normal(gen);
                    continue;
                }
                const auto func_ptr = base_func->known_func(gen);
                if(func_ptr == nullptr) {
                    gen.error("couldn't override function", function);
                    function->code_gen_declare_normal(gen);
                    continue;
                }
                const auto mod = func_ptr->getParent();
                if(mod != gen.module.get()) {
                    // not current module
                    function->code_gen_declare_normal(gen);
                } else {
                    // internal interface, present in current module
                    // we will implement the interface in place, since its present in current module
                    function->set_llvm_data(gen, func_ptr);
                    if(func_ptr->size() == 1) {
                        // remove the stub block present in functions internal to module
                        auto& stubEntry = func_ptr->getEntryBlock();
                        stubEntry.removeFromParent();
                    }
                    const auto final_specifier = is_linkage_public(linked->specifier()) || is_linkage_public(specifier()) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
                    // change the function's linkage to internal
                    func_ptr->setLinkage(final_specifier);
                    gen.createFunctionBlock(func_ptr);
                }
            }
        } else {
            for (auto& function: instantiated_functions()) {
                function->code_gen_declare_normal(gen);
            }
        }
    }
}

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->get_direct_linked_interface();
    const auto struct_def = struct_type ? struct_type->get_direct_linked_struct() : nullptr;
    // struct type is given, but probably primitive
    if(struct_type != nullptr && struct_def == nullptr) {
        for (const auto function: instantiated_functions()) {
            function->code_gen_body(gen);
        }
        if(!linked->is_static()) {
            linked->create_global_vtable(gen, this, struct_type, false);
        }
    } else {
        for (auto& function: instantiated_functions()) {
            code_gen_function(gen, function, linked, struct_def);
        }
        if (linked && !linked->is_static() && struct_def) {
            linked->llvm_global_vtable(gen, struct_def);
        }
    }
}

void ImplDefinition::code_gen_external_declare(Codegen &gen) {
    // const auto linked = interface_type->linked_node()->as_interface_def();
    if(struct_type == nullptr) return;
    // struct type is given, but probably primitive
    const auto struct_def = struct_type->get_direct_linked_struct();
    if(struct_def) {
        // nothing to do here
    } else {
        if(!is_linkage_public(specifier())) {
            return;
        }
        const auto interface = interface_type->get_direct_linked_interface();
        for (auto& function: instantiated_functions()) {
            function->code_gen_external_declare(gen, AccessSpecifier::Public);
        }
        // just declare the global vtable for this primitive impl Again.
        if(!interface->is_static()) {
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
    const auto struct_linked = definition->struct_type ? definition->struct_type->get_direct_linked_struct() : nullptr;
    if(struct_linked) {
        register_use(struct_linked);
        register_use_to_inherited_interfaces(struct_linked);
    }
    attrs.has_implementation = true;
}