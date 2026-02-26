// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ImportSymbol.h"
#include "DependencySymbolInfo.h"

struct LabModule;

struct RemoteImportRequester {
    LabModule* requester;
    DependencySymbolInfo* symbol_info;
};

struct RemoteImport {
    chem::string_view from;
    chem::string_view subdir;
    // either a commit or a version should be given
    chem::string_view version;
    chem::string_view branch;
    chem::string_view commit;
    
    // a remote import can be requested by multiple modules
    std::vector<RemoteImportRequester> requesters;

    // pre-parsed fields filled at registration time
    chem::string_view mod_scope;
    chem::string_view mod_name;
    chem::string_view origin;      // e.g. "github.com"
};