// Copyright (c) Qinetik 2024.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"

void ImplDefinition::code_gen(Codegen &gen) {
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

void ImplDefinition::declare_and_link(SymbolResolver &linker) {
    linked = (InterfaceDefinition *) (linker.find(interface_name));
    if(!linked) {
        linker.error("couldn't find interface by name " + interface_name + " for implementation");
        return;
    }
    if(struct_name.has_value()) {
        struct_linked = (StructDefinition*) (linker.find(struct_name.value()));
        if(!struct_linked) {
            linker.error("couldn't find struct by name " + struct_name.value() + " for implementation of interface " + interface_name);
            return;
        }
    }
    for(auto& func : functions()) {
        if(!func->has_annotation(AnnotationKind::Override)) {
            func->annotations.emplace_back(AnnotationKind::Override);
        }
    }
    linker.scope_start();
    // redeclare functions of interface
    for(auto& func : linked->functions()) {
        func->redeclare_top_level(linker);
    }
    // redeclare everything inside struct
    if(struct_name.has_value()) {
        struct_linked->redeclare_variables_and_functions(linker);
        // TODO currently, struct also inherits the interface
        //   which is being implemented for the struct using impl block
        //   so bringing inherited members into current scope, causes duplicate
//        struct_linked->redeclare_inherited_members(linker);
    }
    MembersContainer::declare_and_link_no_scope(linker);
    linker.scope_end();
}