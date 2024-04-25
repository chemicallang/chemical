// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructMember.h"

InterfaceDefinition::InterfaceDefinition(
        std::string name,
        std::map<std::string, std::unique_ptr<StructMember>> variables,
        std::map<std::string, std::unique_ptr<FunctionDeclaration>> functions
) : name(std::move(name)), variables(std::move(variables)), functions(std::move(functions)) {

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