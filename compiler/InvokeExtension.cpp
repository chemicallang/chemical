// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "cstring"
#include <sstream>
#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/SmallString.h>
#include "SelfInvocation.h"

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));

    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        strcpy(pointers[i], command_args[i].c_str());
        pointers[i][command_args[i].size()] = '\0';
    }

    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    for (size_t i = 0; i < command_args.size(); ++i) {
        free(pointers[i]);
    }
    free(pointers);
    return result;
}

int Codegen::invoke_clang(const std::vector<std::string> &command_args) {
    return chemical_clang_main2(command_args);
}

std::string Codegen::abs_header_path(const std::string& header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(curr_exe_path));
    }

    return ::header_abs_path(system_headers_paths, header);

}

std::string Codegen::headers_dir(const std::string& header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(curr_exe_path));
    }

    return ::headers_dir(system_headers_paths, header);

}