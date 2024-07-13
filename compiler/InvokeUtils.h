// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <string>

/**
 * links different object files / bitcode files using compiler or lld
 */
int link_objects(
        std::vector<std::string>& linkables,
        const std::string& bin_out,
        const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<std::string>& flags, // passed to clang or lld,
        bool use_lld = false,
        bool libc = true
);

/**
 * it'll compile the given file to generate a single .o file using clang
 * it uses -c to generate object file from .c file
 */
int compile_c_file_to_object(
        const char* c_file,
        const char* out_file,
        const std::string& comp_exe_path,
        const std::vector<std::string>& flags
);