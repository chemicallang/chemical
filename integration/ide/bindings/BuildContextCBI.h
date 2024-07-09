// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/LabModuleType.h"
#include "std/chem_string.h"

struct LabBuildContext;

struct LabModule;

struct BuildContextCBI {

    LabModule*(*add_with_type)(BuildContextCBI* self, LabModuleType type, chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len);

    LabBuildContext* instance;

};

void prep_build_context_cbi(BuildContextCBI* cbi);

void bind_build_context_cbi(BuildContextCBI* cbi, LabBuildContext* context);