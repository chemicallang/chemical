// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <string>
#include <string_view>

/**
 * links different object files / bitcode files using compiler or lld
 */
int link_objects(
        std::vector<std::string>& linkables,
        const std::string& bin_out,
        const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<std::string>& flags, // passed to clang or lld,
        const std::vector<std::string>& link_libs,
        const std::string_view& target_triple,
        bool use_lld = false,
        bool libc = true
);

/**
 * it'll compile the given file to generate a single .o file using clang
 * it uses -c to generate object file from .c or a .cpp file
 */
int compile_c_file_to_object(
        const char* c_file,
        const char* out_file,
        const std::string& comp_exe_path,
        const std::vector<std::string>& flags
);