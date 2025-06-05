// Copyright (c) Chemical Language Foundation 2025.

#include "CmdUtils2.h"

char** convert_to_pointers(const std::vector<std::string> &command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));
    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        strcpy(pointers[i], command_args[i].c_str());
        pointers[i][command_args[i].size()] = '\0';
    }
    return pointers;
}

char** convert_to_pointers(const std::span<chem::string_view> &command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));
    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        strcpy(pointers[i], command_args[i].data());
        pointers[i][command_args[i].size()] = '\0';
    }
    return pointers;
}

void free_pointers(char** pointers, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        free(pointers[i]);
    }
    free(pointers);
}
