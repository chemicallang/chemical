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
    ret.append(MembersContainer::representation());
    ret.append("\n}");
    return ret;
}