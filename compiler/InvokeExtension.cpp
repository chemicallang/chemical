// Copyright (c) Qinetik 2024.

#include "Codegen.h"

int chemical_clang_main(int argc, char **argv);

int Codegen::invoke_clang(std::vector<std::string> &command_args) {

    // Convert the vector of strings to an ArrayRef<const char *>
    std::vector<char *> args_cstr;
    args_cstr.reserve(command_args.size());
    for (const std::string& arg : command_args) {
        args_cstr.push_back(const_cast<char*>(arg.c_str()));
    }

    // invocation
    return chemical_clang_main(args_cstr.size(), args_cstr.data());

}

int Codegen::link_objs_as_exes_clang(std::vector<std::string> &obj_files, const std::string &out_path, const std::vector<std::string> &command_args) {
    // TODO

    return 0;
}