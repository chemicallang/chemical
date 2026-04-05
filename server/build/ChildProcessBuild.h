// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

struct BuildContextInformation;
class BasicBuildContext;

int launch_child_build(
    BuildContextInformation& context,
    const std::string_view& lspPath,
    const std::string_view& buildFilePath
);

int compile_lab(
    const std::string_view& exe_path,
    const std::string_view& lab_path,
    std::string& outPayload,
    bool format
);

/**
 * compile a .lab file or a .mod file
 * this method is called in a separate process to handle compilation
 * this method reports to parent process
 */
int compile_lab(
    const std::string_view& exe_path,
    const std::string_view& lab_path,
    bool format,
    std::string_view shmName,
    std::string_view evtChildDone,
    std::string_view evtParentAck
);
