// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/LabModuleType.h"
#include "std/chem_string.h"

struct LabBuildContext;

struct LabModule;

struct LabJob;

struct BuildContextCBI {

    LabModule*(*file_module)(BuildContextCBI* self, chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len);

    LabModule*(*files_module)(BuildContextCBI* self, chem::string* name, chem::string** path, unsigned int path_len, LabModule** dependencies, unsigned int dep_len);

    LabModule*(*c_file_module)(BuildContextCBI* self, chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len);

    LabJob*(*build_exe)(BuildContextCBI* self, chem::string* name, LabModule** dependencies, unsigned int dep_len);

    LabJob*(*build_library)(BuildContextCBI* self, chem::string* name, LabModule** dependencies, unsigned int dep_len);

    void(*build_path)(chem::string* str, BuildContextCBI* self);

    bool(*has_arg)(BuildContextCBI* self, chem::string* name);

    void(*get_arg)(chem::string* str, BuildContextCBI* self, chem::string* name);

    void(*remove_arg)(BuildContextCBI* self, chem::string* name);

    LabBuildContext* instance;

};

void prep_build_context_cbi(BuildContextCBI* cbi);

void bind_build_context_cbi(BuildContextCBI* cbi, LabBuildContext* context);