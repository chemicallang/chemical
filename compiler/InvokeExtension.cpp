// Copyright (c) Qinetik 2024.

#include "Codegen.h"

int chemical_clang_main(int argc, char **argv);

int Codegen::invoke_clang(const std::vector<std::string> &command_args) {

    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));

    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        if (strcpy_s(pointers[i], command_args[i].size() + 1, command_args[i].c_str()) != 0) {
            // Handle strcpy_s failure
            std::cerr << "Failure copying clang command argument";
            // Free memory allocated so far
            for (size_t j = 0; j <= i; ++j) {
                free(pointers[j]);
            }
            free(pointers);
            return -1;
        }
    }

    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    for (size_t i = 0; i < command_args.size(); ++i) {
        free(pointers[i]);
    }
    free(pointers);
    return result;

}

int Codegen::link_objs_as_exes_clang(std::vector<std::string> &obj_files, const std::string &out_path, const std::vector<std::string> &command_args) {
    // TODO

    return 0;
}