// Copyright (c) Chemical Language Foundation 2025.

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
    const std::vector<std::string>& flags,
    const std::string_view& target_triple,
    bool use_tcc
);

/**
 * a helper function, this is for CBI
 */
inline int link_objects(
        const std::string& comp_exe_path,
        std::vector<chem::string>& linkables,
        const std::string& output_path,
        const std::string_view& target_triple
) {
    return link_objects(comp_exe_path, linkables, output_path, {}, target_triple, false);
}