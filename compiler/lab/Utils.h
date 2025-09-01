// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string.h"
#include "integration/libtcc/TCCMode.h"
#include <vector>
#include <string>

class LabBuildCompilerOptions;

int link_objects_tcc(
        const std::string& comp_exe_path,
        std::vector<chem::string>& objects,
        std::vector<chem::string>& link_libs,
        const std::string& output_path,
        TCCMode mode
);

int link_objects_linker(
        const std::string& comp_exe_path,
        std::vector<chem::string>& objects,
        std::vector<chem::string>& link_libs,
        const std::string& output_path,
        const std::string_view& target_triple,
        bool debug_info,
        OutputMode mode,
        bool no_pie,
        bool verbose
);

int link_objects_now(
        bool use_tcc,
        LabBuildCompilerOptions* options,
        std::vector<chem::string>& objects,
        std::vector<chem::string>& link_libs,
        const std::string& output_path,
        const std::string_view& target_triple
);