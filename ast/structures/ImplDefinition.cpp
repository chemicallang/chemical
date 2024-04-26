// Copyright (c) Qinetik 2024.

#include "ImplDefinition.h"
#include "StructMember.h"

void ImplDefinition::code_gen(Codegen &gen) {
    for (auto &function: functions) {
        auto overridden = linked->child(function.second->name);
        if (overridden) {
            auto fn = overridden->as_function();
            if (fn) {
                fn->code_gen_override(gen, function.second.get());
                continue;
            }
        }
    }
}

void ImplDefinition::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
    auto found = linker.current.find(interface_name);
    if(found != linker.current.end()) {
        linked = found->second;
    } else {
        linker.error("couldn't find interface by name " + interface_name + " for implementation");
    }
}