// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string.h"
#include <vector>

/**
 *
 * @param comp_exe_path compiler executable path
 * @param linkables the objects to link .o files
 * @param output_path the binary output path
 * @param flags the linker flags, passed to both tcc and clang, so check for support before including
 */
int link_objects(
    const std::string& comp_exe_path,
    std::vector<chem::string>& linkables,
    const std::string& output_path,
    const std::vector<std::string>& flags
);

/**
 * a helper function, this is for CBI
 */
inline int link_objects(
        const std::string& comp_exe_path,
        std::vector<chem::string>& linkables,
        const std::string& output_path
) {
    return link_objects(comp_exe_path, linkables, output_path, {});
}