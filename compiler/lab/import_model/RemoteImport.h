// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ImportSymbol.h"
#include "DependencySymbolInfo.h"

struct LabModule;

struct RemoteImport {
    chem::string_view from;
    chem::string_view subdir;
    // either a commit or a version should be given
    chem::string_view version;
    chem::string_view branch;
    chem::string_view commit;
    DependencySymbolInfo symbol_info;
    LabModule* requester;
};