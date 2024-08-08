// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/LabModuleType.h"
#include "std/chem_string.h"

struct LabBuildContext;

struct LabModule;

struct LabJob;

struct ModuleArrayRef {
    LabModule** ptr;
    size_t size;
};

struct BuildContextCBI {

    LabModule*(*files_module)(BuildContextCBI* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies);

    LabModule*(*chemical_files_module)(BuildContextCBI* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies);

    LabModule*(*c_file_module)(BuildContextCBI* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies);

    LabModule*(*object_module)(BuildContextCBI* self, chem::string* name, chem::string* path);

    LabJob*(*translate_to_chemical)(BuildContextCBI* self, chem::string* c_path, chem::string* output_path);

    LabJob*(*translate_mod_to_c)(BuildContextCBI* self, LabModule* chem_path, chem::string* output_path);

    LabJob*(*build_exe)(BuildContextCBI* self, chem::string* name, ModuleArrayRef* dependencies);

    LabJob*(*build_dynamic_lib)(BuildContextCBI* self, chem::string* name, ModuleArrayRef* dependencies);

    void(*add_object)(BuildContextCBI* self, LabJob* job, chem::string* path);

    void(*build_path)(chem::string* str, BuildContextCBI* self);

    bool(*has_arg)(BuildContextCBI* self, chem::string* name);

    void(*get_arg)(chem::string* str, BuildContextCBI* self, chem::string* name);

    void(*remove_arg)(BuildContextCBI* self, chem::string* name);

    int(*launch_executable)(BuildContextCBI* self, chem::string* path, bool same_window);

    void(*on_finished)(BuildContextCBI* self, void(*lambda)(void*), void* data);

    LabBuildContext* instance;

};

void prep_build_context_cbi(BuildContextCBI* cbi);

void bind_build_context_cbi(BuildContextCBI* cbi, LabBuildContext* context);