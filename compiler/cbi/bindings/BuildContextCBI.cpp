// Copyright (c) Chemical Language Foundation 2025.

#include <span>
#include "BuildContextCBI.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/ProcessUtils.h"
#include "utils/PathUtils.h"
#include "compiler/lab/Utils.h"
#include "compiler/InvokeUtils.h"
#include "preprocess/ImportPathHandler.h"
#include "CBIUtils.h"
#include "parser/model/CompilerBinder.h"

#ifdef COMPILER_BUILD
int llvm_ar_main2(const std::span<chem::string_view> &command_args);
#endif

LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies) {
    return self->files_module(*scope_name, *name, path, path_len, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextchemical_files_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies) {
    return self->chemical_files_module(*scope_name, *name, path, path_len, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextchemical_dir_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies) {
    return self->chemical_dir_module(*scope_name, *name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextc_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies) {
    return self->c_file_module(*scope_name, *name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextcpp_file_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path, ModuleSpan* dependencies) {
    return self->cpp_file_module(*scope_name, *name, path, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextobject_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view* path) {
    return self->obj_file_module(*scope_name, *name, path);
}

bool BuildContextadd_compiler_interface(LabBuildContext* self, LabModule* module, chem::string_view* interface) {
    auto& maps = self->binder.interface_maps;
    auto found = maps.find(*interface);
    if(found != maps.end()) {
        module->compiler_interfaces.emplace_back(found->second);
        return true;
    } else {
        return false;
    }
}

void BuildContextresolve_import_path(PathResolutionResult* result, LabBuildContext* self, chem::string_view* base_path, chem::string_view* path) {
    init_chem_string(&result->error);
    init_chem_string(&result->path);
    auto repResult = self->handler.resolve_import_path(base_path->str(), path->str());
    if(!repResult.error.empty()) {
        result->error.append(repResult.error);
    }
    if(!repResult.replaced.empty()) {
        result->path.append(repResult.replaced);
    }
}

void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string_view* header) {
    module->headers.emplace_back(*header);
}

LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string_view* output_path) {
    return self->translate_to_chemical(module, output_path);
}

LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies, chem::string_view* output_dir) {
    return self->translate_to_c(name, dependencies->ptr, dependencies->size, output_dir);
}

LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies) {
    return self->build_exe(name, dependencies->ptr, dependencies->size);
}

LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string_view* name, ModuleSpan* dependencies) {
    return self->build_dynamic_lib(name, dependencies->ptr, dependencies->size);
}

LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string_view* name, LabModule* entry, ModuleSpan* dependencies) {
    return self->build_cbi(name, dependencies->ptr, dependencies->size, entry);
}

void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string_view* path) {
    job->linkables.emplace_back(*path);
}

bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string_view* alias, chem::string_view* path) {
    return self->declare_user_alias(job, alias->str(), path->str());
}

void BuildContextbuild_path(chem::string* str, LabBuildContext* self) {
    init_chem_string(str)->append(self->options->build_dir);
}

bool BuildContexthas_arg(LabBuildContext* self, chem::string_view* name) {
    return self->has_arg(name->str());
}

void BuildContextget_arg(chem::string* str, LabBuildContext* self, chem::string_view* name) {
    return self->get_arg(*init_chem_string(str), name->str());
}

void BuildContextremove_arg(LabBuildContext* self, chem::string_view* name) {
    return self->remove_arg(name->str());
}

bool BuildContextdefine(LabBuildContext* self, LabJob* job, chem::string_view* name) {
    auto def_name = name->str();
    auto& definitions = job->definitions;
    auto got = definitions.find(def_name);
    if(got == definitions.end()) {
        definitions[std::move(def_name)] = true;
        return true;
    } else {
        return false;
    }
}

bool BuildContextundefine(LabBuildContext* self, LabJob* job, chem::string_view* name) {
    auto& definitions = job->definitions;
    auto got = definitions.find(name->str());
    if(got != definitions.end()) {
        definitions.erase(got);
        return true;
    } else {
        return false;
    }
}

int BuildContextlaunch_executable(LabBuildContext* self, chem::string_view* path, bool same_window) {
    auto copied = absolute_path(path->view());
    if(same_window) {
        copied = '\"' + copied + '\"';
    }
    return launch_executable(copied.data(), same_window);
}

void BuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data) {
    self->on_finished = lambda;
    self->on_finished_data = data;
}

int BuildContextlink_objects(LabBuildContext* self, StringViewSpan* string_arr, chem::string_view* output_path) {
    std::vector<chem::string> linkables;
    for(auto i = 0; i < string_arr->size; i++) {
        linkables.emplace_back(chem::string::make_view(string_arr->ptr[i]));
    }
    return link_objects(self->options->exe_path, linkables, output_path->str(), self->options->target_triple);
}

int BuildContextinvoke_dlltool(LabBuildContext* self, StringViewSpan* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<chem::string_view> arr;
    arr.emplace_back("dlltool");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i]);
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_ranlib(LabBuildContext* self, StringViewSpan* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<chem::string_view> arr;
    arr.emplace_back("ranlib");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i]);
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_lib(LabBuildContext* self, StringViewSpan* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<chem::string_view> arr;
    arr.emplace_back("lib");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i]);
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

int BuildContextinvoke_ar(LabBuildContext* self, StringViewSpan* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<chem::string_view> arr;
    arr.emplace_back("ar");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i]);
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}