// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructMember.h"

#ifdef COMPILER_BUILD

void InterfaceDefinition::code_gen(Codegen &gen) {
    for(auto& function : functions) {
        function.second->code_gen_interface(gen);
    }
}

#endif

InterfaceDefinition::InterfaceDefinition(
        std::string name
) : name(std::move(name)) {

}

std::string InterfaceDefinition::representation() const {
    std::string ret("interface " + name + " {\n");
    int i = 0;
    for (const auto &field: variables) {
        ret.append(field.second->representation());
        if (i < variables.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    i = 0;
    for (const auto &field: functions) {
        ret.append(field.second->representation());
        if (i < variables.size() - 1) {
            ret.append(1, '\n');
        }
        i++;
    }
    ret.append("\n}");
    return ret;
}