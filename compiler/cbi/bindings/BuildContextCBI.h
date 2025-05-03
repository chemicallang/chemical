// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include "std/chem_string.h"
#include "std/chem_string_view.h"
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

struct StringViewSpan {
    chem::string_view* ptr;
    size_t size;
};

extern "C" {

    struct PathResolutionResult {
        chem::string path;
        chem::string error;
    };

    LabModule* BuildContextmodule_from_directory(LabBuildContext* self, chem::string_view* path, chem::string_view* scope_name, chem::string_view* mod_name, chem::string* error_msg);

    LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies);

    LabModule* BuildContextchemical_files_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies);

    LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextobject_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path);

    bool BuildContextadd_compiler_interface(LabBuildContext* self, LabModule* module, chem::string_view* interface);

    void BuildContextresolve_import_path(PathResolutionResult* result, LabBuildContext* self, chem::string_view* base_path, chem::string_view* path);

    void BuildContextresolve_native_lib_path(PathResolutionResult* result, LabBuildContext* self, chem::string_view* scope_name, chem::string_view* mod_name);

    void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string_view* header);

    LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string_view* output_path);

    LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies, chem::string_view* output_dir);

    LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies);

    LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies);

    LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies);

    bool BuildContextindex_cbi_fn(LabBuildContext* self, LabJob* job, chem::string_view* key, chem::string_view* fn_name, int func_type);

    void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string_view* path);

    bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string_view* alias, chem::string_view* path);

    void BuildContextbuild_path(chem::string* str, LabBuildContext* self);

    bool BuildContexthas_arg(LabBuildContext* self, chem::string_view* name);

    void BuildContextget_arg(chem::string* str, LabBuildContext* self, chem::string_view* name);

    void BuildContextremove_arg(LabBuildContext* self, chem::string_view* name);

    bool BuildContextdefine(LabBuildContext* self, LabJob* job, chem::string_view* name);

    bool BuildContextundefine(LabBuildContext* self, LabJob* job, chem::string_view* name);

    int AppBuildContextlaunch_executable(LabBuildContext* self, chem::string_view* path, bool same_window);

    void AppBuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data);

    int BuildContextlink_objects(LabBuildContext* self, StringViewSpan* string_arr, chem::string_view* output_path);

    int BuildContextinvoke_dlltool(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_ranlib(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_lib(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_ar(LabBuildContext* self, StringViewSpan* string_arr);

}