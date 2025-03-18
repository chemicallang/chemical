// Copyright (c) Chemical Language Foundation 2025.

#include "BuildContextCBI.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/ProcessUtils.h"
#include "utils/PathUtils.h"
#include "compiler/lab/Utils.h"
#include "compiler/InvokeUtils.h"
#include "preprocess/ImportPathHandler.h"
#include "CBIUtils.h"

#ifdef COMPILER_BUILD
int llvm_ar_main2(const std::vector<std::string> &command_args);
#endif

LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies) {
    return self->files_module(name, path, path_len, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextchemical_files_module(LabBuildContext* self, chem::string* name, chem::string** path, unsigned int path_len, ModuleArrayRef* dependencies) {
    return self->chemical_files_module(name, path, path_len, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) {
    return self->chemical_dir_module(name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) {
    return self->c_file_module(name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string* name, chem::string* path, ModuleArrayRef* dependencies) {
    return self->cpp_file_module(name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextobject_module(LabBuildContext* self, chem::string* name, chem::string* path) {
    return self->obj_file_module(name, path);
}

void BuildContextresolve_import_path(PathResolutionResult* result, LabBuildContext* self, chem::string* base_path, chem::string* path) {
    init_chem_string(&result->error);
    init_chem_string(&result->path);
    auto repResult = self->handler.resolve_import_path(base_path->to_std_string(), path->to_std_string());
    if(!repResult.error.empty()) {
        result->error.append(repResult.error);
    }
    if(!repResult.replaced.empty()) {
        result->path.append(repResult.replaced);
    }
}

void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string* header) {
    module->headers.emplace_back(header->to_view());
}

void BuildContextinclude_file(LabBuildContext* self, LabModule* module, chem::string* abs_path) {
    module->includes.emplace_back(abs_path->to_view());
}

LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string* output_path) {
    return self->translate_to_chemical(module, output_path);
}

LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies, chem::string* output_dir) {
    return self->translate_to_c(name, dependencies->ptr, dependencies->size, output_dir);
}

LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies) {
    return self->build_exe(name, dependencies->ptr, dependencies->size);
}

LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string* name, ModuleArrayRef* dependencies) {
    return self->build_dynamic_lib(name, dependencies->ptr, dependencies->size);
}

LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string* name, LabModule* entry, ModuleArrayRef* dependencies) {
    return self->build_cbi(name, dependencies->ptr, dependencies->size, entry);
}

void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string* path) {
    job->linkables.emplace_back(path->copy());
}

bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string* alias, chem::string* path) {
    return self->declare_user_alias(job, alias->to_std_string(), path->to_std_string());
}

void BuildContextbuild_path(chem::string* str, LabBuildContext* self) {
    init_chem_string(str)->append(self->build_dir);
}

bool BuildContexthas_arg(LabBuildContext* self, chem::string* name) {
    return self->has_arg(name);
}

void BuildContextget_arg(chem::string* str, LabBuildContext* self, chem::string* name) {
    return self->get_arg(init_chem_string(str), name);
}

void BuildContextremove_arg(LabBuildContext* self, chem::string* name) {
    return self->remove_arg(name);
}

bool BuildContextdefine(LabBuildContext* self, LabJob* job, chem::string* name) {
    auto def_name = name->to_std_string();
    auto& definitions = job->definitions;
    auto got = definitions.find(def_name);
    if(got == definitions.end()) {
        definitions[def_name] = true;
        return true;
    } else {
        return false;
    }
}

bool BuildContextundefine(LabBuildContext* self, LabJob* job, chem::string* name) {
    auto def_name = name->to_std_string();
    auto& definitions = job->definitions;
    auto got = definitions.find(def_name);
    if(got != definitions.end()) {
        definitions.erase(got);
        return true;
    } else {
        return false;
    }
}

int BuildContextlaunch_executable(LabBuildContext* self, chem::string* path, bool same_window) {
    auto copied = path->to_std_string();
    copied = absolute_path(copied);
    if(same_window) {
        copied = '\"' + copied + '\"';
    }
    return launch_executable(copied.data(), same_window);
}

void BuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data) {
    self->on_finished = lambda;
    self->on_finished_data = data;
}

int BuildContextlink_objects(LabBuildContext* self, StringArrayRef* string_arr, chem::string* output_path) {
    std::vector<chem::string> linkables;
    for(auto i = 0; i < string_arr->size; i++) {
        linkables.emplace_back(string_arr->ptr[i].copy());
    }
    return link_objects(self->options->exe_path, linkables, output_path->to_std_string(), self->options->target_triple);
}

int BuildContextinvoke_dlltool(LabBuildContext* self, StringArrayRef* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<std::string> arr;
    arr.emplace_back("dlltool");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i].to_std_string());
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_ranlib(LabBuildContext* self, StringArrayRef* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<std::string> arr;
    arr.emplace_back("ranlib");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i].to_std_string());
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_lib(LabBuildContext* self, StringArrayRef* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<std::string> arr;
    arr.emplace_back("lib");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i].to_std_string());
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_ar(LabBuildContext* self, StringArrayRef* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<std::string> arr;
    arr.emplace_back("ar");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i].to_std_string());
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}