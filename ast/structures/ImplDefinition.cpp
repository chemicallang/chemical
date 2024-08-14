// Copyright (c) Qinetik 2024.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"

void ImplDefinition::code_gen(Codegen &gen) {
    const auto linked = interface_type->linked_node();
    auto& interface_name = interface_type->ref_name();
    auto unimplemented = gen.unimplemented_interfaces.find(interface_name);
    if(unimplemented == gen.unimplemented_interfaces.end()) {
        gen.error("Couldn't find interface with name '" + interface_name + "' for implementation");
        return;
    }
//    auto interface = (InterfaceDefinition*) linked;
    for (auto &function: functions()) {
        auto unimp_func = unimplemented->second.find(function->name);
        if(unimp_func == unimplemented->second.end()) {
            gen.error("Couldn't find function in interface " + interface_name);
            continue;
        }
        if(!unimp_func->second) {
            gen.error("Function '" + function->name + "' in interface '" + interface_name + "' has already been implemented");
            continue;
        }
        auto overridden = linked->child(function->name);
        if (overridden) {
            auto fn = overridden->as_function();
            if (fn) {
                function->code_gen_override(gen, fn);
                unimp_func->second = nullptr;
            }
        }
//        if(!interface->has_implemented(function.second->name)) {
//            auto overridden = linked->child(function.second->name);
//            if (overridden) {
//                auto fn = overridden->as_function();
//                if (fn) {
//                    fn->code_gen_override(gen, function.second.get());
//                    interface->set_implemented(function.second->name, true);
//                }
//            }
//        } else {
//            gen.error("Function '" + function.second->name + "' in interface '" + interface_name + "' has already been implemented");
//        }
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


void ImplDefinition::declare_and_link(SymbolResolver &linker) {
    interface_type->link(linker, interface_type);
    if(struct_type) {
        struct_type->link(linker, struct_type);
    }
    auto& interface_name = interface_type->ref_name();
    auto linked = interface_type->linked_node()->as_interface_def();
    if(!linked) {
        linker.error("couldn't find interface by name " + interface_name + " for implementation");
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
            func->redeclare_top_level(linker);
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