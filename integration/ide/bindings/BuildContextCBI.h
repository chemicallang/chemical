// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string.h"

struct LabBuildContext;

struct LabModule;

struct BuildContextCBI {

    LabModule*(*dir_module)(BuildContextCBI* self, chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len);

    LabBuildContext* instance;

};

void prep_build_context_cbi(BuildContextCBI* cbi);

void bind_build_context_cbi(BuildContextCBI* cbi, LabBuildContext* context);