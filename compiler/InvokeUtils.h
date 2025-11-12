// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include "std/chem_string_view.h"
#include "std/chem_string.h"

struct LinkFlags {

    bool no_pie = false;

    bool debug_info = false;

    bool verbose = false;

};

int lld_link_objects(
        std::vector<chem::string>& linkables,
        const std::string& bin_out,
        const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<chem::string>& link_libs,
        const std::string_view& target_triple,
        LinkFlags& flags
);

int clang_link_objects(
        std::vector<chem::string>& linkables,
        const std::string& bin_out,
        const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<chem::string>& link_libs,
        const std::string_view& target_triple,
        LinkFlags& flags
);

int link_objects_linker(
        const std::string& comp_exe_path,
        std::vector<chem::string>& objects,
        std::vector<chem::string>& link_libs,
        const std::string& output_path,
        const std::string_view& target_triple,
        LinkFlags& flags,
        bool use_lld
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