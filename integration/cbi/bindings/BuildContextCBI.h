// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"
#include <cstddef>

struct LabBuildContext;

struct LabModule;

struct LabJob;

struct ModuleArrayRef {
    LabModule** ptr;
    size_t size;
};

struct StringArrayRef {
    chem::string* ptr;
    size_t size;
};

extern "C" {

    LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies);

    LabModule* BuildContextchemical_files_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies);

    LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies);

    LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies);

    LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies);

    LabModule* BuildContextobject_module(LabBuildContext* self, chem::string* name, chem::string* path);

    void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string* header);

    LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string* output_path);

    LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies, chem::string* output_dir);

    LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies);

    LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies);

    LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string* name, LabModule* entry, ModuleArrayRef* dependencies);

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

    int BuildContextlink_objects(LabBuildContext* self, StringArrayRef* string_arr, chem::string* output_path);

    int BuildContextinvoke_dlltool(LabBuildContext* self, StringArrayRef* string_arr);

    int BuildContextinvoke_ranlib(LabBuildContext* self, StringArrayRef* string_arr);

    int BuildContextinvoke_lib(LabBuildContext* self, StringArrayRef* string_arr);

    int BuildContextinvoke_ar(LabBuildContext* self, StringArrayRef* string_arr);

}