// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include "std/chem_string.h"
#include <cstddef>

struct LabBuildContext;

struct LabModule;

struct LabJob;

struct ModuleSpan {
    LabModule** ptr;
    size_t size;
};

struct StringSpan {
    chem::string* ptr;
    size_t size;
};

extern "C" {

    struct PathResolutionResult {
        chem::string path;
        chem::string error;
    };

    LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleSpan* dependencies);

    LabModule* BuildContextchemical_files_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleSpan* dependencies);

    LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleSpan* dependencies);

    LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleSpan* dependencies);

    LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleSpan* dependencies);

    LabModule* BuildContextobject_module(LabBuildContext* self, chem::string* name, chem::string* path);

    void BuildContextresolve_import_path(PathResolutionResult* result, LabBuildContext* self, chem::string* base_path, chem::string* path);

    void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string* header);

    void BuildContextinclude_file(LabBuildContext* self, LabModule* module, chem::string* abs_path);

    LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string* output_path);

    LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string* name, ModuleSpan* dependencies, chem::string* output_dir);

    LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string* name, ModuleSpan* dependencies);

    LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string* name, ModuleSpan* dependencies);

    LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string* name, LabModule* entry, ModuleSpan* dependencies);

    void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string* path);

    bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string* alias, chem::string* path);

    void BuildContextbuild_path(chem::string* str, LabBuildContext* self);

    bool BuildContexthas_arg(LabBuildContext* self, chem::string* name);

    void BuildContextget_arg(chem::string* str, LabBuildContext* self, chem::string* name);

    void BuildContextremove_arg(LabBuildContext* self, chem::string* name);

    bool BuildContextdefine(LabBuildContext* self, LabJob* job, chem::string* name);

    bool BuildContextundefine(LabBuildContext* self, LabJob* job, chem::string* name);

    int BuildContextlaunch_executable(LabBuildContext* self, chem::string* path, bool same_window);

    void BuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data);

    int BuildContextlink_objects(LabBuildContext* self, StringSpan* string_arr, chem::string* output_path);

    int BuildContextinvoke_dlltool(LabBuildContext* self, StringSpan* string_arr);

    int BuildContextinvoke_ranlib(LabBuildContext* self, StringSpan* string_arr);

    int BuildContextinvoke_lib(LabBuildContext* self, StringSpan* string_arr);

    int BuildContextinvoke_ar(LabBuildContext* self, StringSpan* string_arr);

}