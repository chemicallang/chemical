// Copyright (c) Qinetik 2024.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->linked_node()->as_interface_def();
    const auto struct_def = struct_type ? struct_type->linked_struct_def() : nullptr;
    for (auto& function: functions()) {
        auto overridden = linked->get_func_with_signature(function.get());
        if (overridden.first) {
            const auto interface_def = overridden.first->as_interface_def();
            if(!interface_def) {
                gen.error("failed to override function in impl, because function not present in an interface above", function.get());
                continue;
            }
            if(struct_def) {
                const auto& use = interface_def->users[struct_def];
                auto found = use.find(overridden.second);
                if(found == use.end()) {
                    gen.error("failed to override function in impl because declaration not found", function.get());
                    continue;
                }
                function->set_llvm_data(found->second, found->second->getFunctionType());
                function->code_gen_override(gen, found->second);
            } else {
                function->code_gen_override_declare(gen, overridden.second);
                function->code_gen_override(gen, overridden.second->llvm_func());
            }
        } else {
            gen.error("failed to override function in impl because not found", function.get());
        }
    }
    if(linked && struct_def) {
        linked->llvm_global_vtable(gen, struct_def);
    }
}

#endif

ImplDefinition::ImplDefinition(
        ASTNode* parent_node
) : parent_node(parent_node) {

}

ImplDefinition::ImplDefinition(
    std::unique_ptr<BaseType> interface_type,
    std::unique_ptr<BaseType> struct_type,
    ASTNode* parent_node
) : interface_type(std::move(interface_type)), struct_type(std::move(struct_type)), parent_node(parent_node) {

}


void ImplDefinition::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    interface_type->link(linker, interface_type);
    if(struct_type) {
        struct_type->link(linker, struct_type);
    }
    auto& interface_name = interface_type->ref_name();
    auto linked = interface_type->linked_node()->as_interface_def();
    if(!linked) {
        linker.error("couldn't find interface by name " + interface_name + " for implementation", this);
        return;
    }
    for(auto& func : functions()) {
        if(!func->has_annotation(AnnotationKind::Override)) {
            func->annotations.emplace_back(AnnotationKind::Override);
        }
    }
    linker.scope_start();
    const auto struct_linked = struct_type ? struct_type->linked_struct_def() : nullptr;
    const auto overrides_interface = struct_linked && struct_linked->does_override(linked);
    if(!overrides_interface) {
        for (auto& func: linked->functions()) {
            func->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) func);
        }
    }
    // redeclare everything inside struct
    if(struct_linked) {
        struct_linked->redeclare_inherited_members(linker);
        struct_linked->redeclare_variables_and_functions(linker);
    }
    MembersContainer::declare_and_link_no_scope(linker);
    linker.scope_end();
    linked->register_impl(this);
    if(struct_linked) {
        // adding all methods of this implementation to struct
        struct_linked->adopt(this);
    }
}

void InterfaceDefinition::register_impl(ImplDefinition* definition) {
    const auto struct_linked = definition->struct_type ? definition->struct_type->linked_struct_def() : nullptr;
    if(struct_linked) {
        register_use(struct_linked);
        register_use_to_inherited_interfaces(struct_linked);
    }
    has_implementation = true;
}