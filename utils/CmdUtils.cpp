// Copyright (c) Chemical Language Foundation 2025.

#include "CmdUtils2.h"
#include "std/chem_string.h"

char** chem_string_cmd_pointers(chem::string& front, const std::span<chem::string>& command_args) {
    char** pointers = static_cast<char **>(malloc((command_args.size() + 1) * sizeof(char*)));
    pointers[0] = front.mutable_data();
    auto cmd_pointers = pointers + 1;
    for (size_t i = 0; i < command_args.size(); ++i) {
        cmd_pointers[i] = command_args[i].mutable_data();
    }
    return pointers;
}

char** chem_string_cmd_pointers(const std::span<chem::string>& command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = command_args[i].mutable_data();
    }
    return pointers;
}

void free_chem_string_cmd_pointers(char** pointers, size_t size) {
    free(pointers);
}