// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIUtils.h"
#include "std/chem_string.h"
#include "std/chem_string_view.h"
#include <cstddef>

class LabBuildContext;

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

struct RemoteImportSymbolPartsSpan {
    chem::string_view* ptr;
    size_t size;
};

struct RemoteImportSymbolCBI {
    RemoteImportSymbolPartsSpan symbol;
    chem::string_view alias;
};

struct RemoteImportSymbolCBISpan {
    RemoteImportSymbolCBI* ptr;
    size_t size;
};

struct RemoteImportCBI {
    chem::string_view from;
    chem::string_view subdir;
    chem::string_view version;
    chem::string_view branch;
    chem::string_view commit;
    chem::string_view alias;
    RemoteImportSymbolCBISpan symbols;
    uint64_t location;
};

class AnnotationController;

extern "C" {

    struct PathResolutionResult {
        chem::string path;
        chem::string error;
    };

    AnnotationController* BuildContextgetAnnotationController(LabBuildContext* self);

    LabModule* BuildContextnew_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, ModuleSpan* dependencies);

    LabModule* BuildContextget_cached(LabBuildContext* self, LabJob* job, chem::string_view* scope_name, chem::string_view* name);

    void BuildContextset_cached(LabBuildContext* self, LabJob* job, LabModule* module);

    void BuildContextadd_path(LabBuildContext* self, LabModule* module, chem::string_view* path);

    void BuildContextadd_module(LabBuildContext* self, LabJob* job, LabModule* module);

    LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies);

    LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies);

    LabModule* BuildContextobject_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path);

    void BuildContextput_job_before(LabBuildContext* self, LabJob* newJob, LabJob* existingJob);

    void BuildContextlink_system_lib(LabBuildContext* self, LabJob* job, chem::string_view* name, LabModule* module);

    bool BuildContextadd_compiler_interface(LabBuildContext* self, LabModule* module, chem::string_view* interface);

    void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string_view* header);

    LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string_view* output_path);

    LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string_view* name, chem::string_view* output_path);

    LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string_view* name);

    LabJob* BuildContextrun_jit_exe(LabBuildContext* self, chem::string_view* name);

    LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string_view* name);

    LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string_view* name);

    void BuildContextset_environment_testing(LabBuildContext* self, LabJob* job, bool value);

    bool BuildContextindex_cbi_fn(LabBuildContext* self, LabJob* job, chem::string_view* key, chem::string_view* fn_name, int func_type);

    void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string_view* path);

    bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string_view* alias, chem::string_view* path);

    void BuildContextbuild_path(chem::string_view* str, LabBuildContext* self);

    bool BuildContexthas_arg(LabBuildContext* self, chem::string_view* name);

    void BuildContextget_arg(chem::string_view* str, LabBuildContext* self, chem::string_view* name);

    void BuildContextremove_arg(LabBuildContext* self, chem::string_view* name);

    bool BuildContextdefine(LabBuildContext* self, LabJob* job, chem::string_view* name);

    bool BuildContextundefine(LabBuildContext* self, LabJob* job, chem::string_view* name);

    int AppBuildContextlaunch_executable(LabBuildContext* self, chem::string_view* path, bool same_window);

    void AppBuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data);

    int BuildContextinvoke_dlltool(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_ranlib(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_lib(LabBuildContext* self, StringViewSpan* string_arr);

    int BuildContextinvoke_ar(LabBuildContext* self, StringViewSpan* string_arr);

    void BuildContextfetch_job_dependency(LabBuildContext* self, LabJob* job, RemoteImportCBI* dep);

    void BuildContextfetch_mod_dependency(LabBuildContext* self, LabJob* job, LabModule* mod, RemoteImportCBI* dep);


}
