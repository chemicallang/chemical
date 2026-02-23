// Copyright (c) Chemical Language Foundation 2025.

#include <span>
#include "BuildContextCBI.h"
#include "compiler/lab/LabBuildContext.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "utils/ProcessUtils.h"
#include "utils/PathUtils.h"
#include "compiler/lab/Utils.h"
#include "compiler/InvokeUtils.h"
#include "preprocess/ImportPathHandler.h"
#include "CBIUtils.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/frontend/AnnotationController.h"
#include "ast/base/GlobalInterpretScope.h"

#ifdef COMPILER_BUILD
int llvm_ar_main2(const std::span<chem::string>& command_args);
#endif

AnnotationController* BuildContextgetAnnotationController(LabBuildContext* self) {
    return &self->compiler.controller;
}

LabModule* BuildContextnew_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, ModuleSpan* dependencies) {
    return self->new_module(*scope_name, *name, dependencies->ptr, dependencies->size);
}

LabModule* BuildContextget_cached(LabBuildContext* self, LabJob* job, chem::string_view* scope_name, chem::string_view* name) {
    return self->get_cached(job, *scope_name, *name);
}

void BuildContextset_cached(LabBuildContext* self, LabJob* job, LabModule* module) {
    self->set_exists_in_cache(job, module);
}

void BuildContextadd_path(LabBuildContext* self, LabModule* module, chem::string_view* path) {
    module->paths.emplace_back(*path);
}

void BuildContextadd_module(LabBuildContext* self, LabJob* job, LabModule* module) {
    job->add_dependency(module);
}

LabModule* BuildContextfiles_module(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, chem::string_view** path, unsigned int path_len, ModuleSpan* dependencies) {
    return self->files_module(*scope_name, *name, path, path_len, dependencies->ptr, dependencies->size);
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

void BuildContextput_job_before(LabBuildContext* self, LabJob* newJob, LabJob* existingJob) {
    self->put_job_before(newJob, existingJob);
}

void BuildContextlink_system_lib(LabBuildContext* self, LabJob* job, chem::string_view* name, LabModule* module) {
    job->link_libs.emplace_back(*name);
}

bool BuildContextadd_compiler_interface(LabBuildContext* self, LabModule* module, chem::string_view* interface) {
    auto& maps = self->binder.interface_maps;
    auto found = maps.find(*interface);
    if(found != maps.end()) {
        module->compiler_interfaces.emplace_back(found->second);
#ifdef LSP_BUILD
        module->compiler_interfaces_str.emplace_back(chem::string(*interface));
#endif
        return true;
    } else {
        return false;
    }
}

// TODO: strings are required to be returned, cbi not supporting
void BuildContextresolve_import_path(LabBuildContext* self, chem::string_view* base_path, chem::string_view* path) {
    PathResolutionResult result{ chem::string(), chem::string() };
    auto repResult = self->handler.resolve_import_path(base_path->view(), path->view());
    if(!repResult.error.empty()) {
        result.error.append(repResult.error);
    }
    if(!repResult.replaced.empty()) {
        result.path.append(repResult.replaced);
    }
}

// TODO: strings are required to be returned, cbi not supporting
void BuildContextresolve_native_lib_path(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* mod_name) {
    PathResolutionResult result{ chem::string(), chem::string() };
    auto repResult = self->handler.resolve_lib_dir_path(*scope_name, *mod_name);
    if(!repResult.error.empty()) {
        result.error.append(repResult.error);
    }
    if(!repResult.replaced.empty()) {
        result.path.append(repResult.replaced);
    }
}

void BuildContextinclude_header(LabBuildContext* self, LabModule* module, chem::string_view* header) {
    module->headers.emplace_back(*header);
}

LabJob* BuildContexttranslate_to_chemical(LabBuildContext* self, LabModule* module, chem::string_view* output_path) {
    return self->translate_to_chemical(module, output_path);
}

LabJob* BuildContexttranslate_to_c(LabBuildContext* self, chem::string_view* name, chem::string_view* output_path) {
    return self->translate_to_c(name, output_path);
}

LabJob* BuildContextbuild_exe(LabBuildContext* self, chem::string_view* name) {
    return self->build_exe(name);
}

LabJob* BuildContextrun_jit_exe(LabBuildContext* self, chem::string_view* name) {
    return self->run_jit_exe(name);
}

LabJob* BuildContextbuild_dynamic_lib(LabBuildContext* self, chem::string_view* name) {
    return self->build_dynamic_lib(name);
}

LabJob* BuildContextbuild_cbi(LabBuildContext* self, chem::string_view* name) {
    return self->build_cbi(name);
}

void BuildContextset_environment_testing(LabBuildContext* self, LabJob* job, bool value) {
    if(value) {
        self->compiler.controller.ensure_test_resources();
    }
    job->target_data.test = value;
}

bool BuildContextindex_cbi_fn(LabBuildContext* self, LabJob* job, chem::string_view* key, chem::string_view* fn_name, int func_type) {
    if(job->type == LabJobType::CBI && func_type >= 0) {
        ((LabJobCBI*) job)->indexes.emplace_back(chem::string(*key), chem::string(*fn_name), static_cast<CBIFunctionType>(func_type));
        return true;
    } else {
        return false;
    }
}

void BuildContextadd_object(LabBuildContext* self, LabJob* job, chem::string_view* path) {
    job->objects.emplace_back(*path);
}

bool BuildContextdeclare_alias(LabBuildContext* self, LabJob* job, chem::string_view* alias, chem::string_view* path) {
    return self->declare_user_alias(job, alias->str(), path->str());
}

void BuildContextbuild_path(chem::string_view* view, LabBuildContext* self) {
    auto& str = self->compiler.options->build_dir;
    new (view) chem::string_view(str.data(), str.size());
}

bool BuildContexthas_arg(LabBuildContext* self, chem::string_view* name) {
    return self->has_arg(name->str());
}

void BuildContextget_arg(chem::string_view* str, LabBuildContext* self, chem::string_view* name) {
    *str = self->get_arg(name->str());
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

int AppBuildContextlaunch_executable(LabBuildContext* self, chem::string_view* path, bool same_window) {
    auto copied = absolute_path(path->view());
    if(same_window) {
        copied = '\"' + copied + '\"';
    }
    return launch_executable(copied.data(), same_window);
}

void AppBuildContexton_finished(LabBuildContext* self, void(*lambda)(void*), void* data) {
    self->on_finished = lambda;
    self->on_finished_data = data;
}

int BuildContextinvoke_dlltool(LabBuildContext* self, StringViewSpan* string_arr) {
#ifdef COMPILER_BUILD
    std::vector<chem::string> arr;
    arr.reserve(string_arr->size + 1);
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
    std::vector<chem::string> arr;
    arr.reserve(string_arr->size + 1);
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
    std::vector<chem::string> arr;
    arr.reserve(string_arr->size + 1);
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
    std::vector<chem::string> arr;
    arr.reserve(string_arr->size + 1);
    arr.emplace_back("ar");
    for(auto i = 0; i < string_arr->size; i++) {
        arr.emplace_back(string_arr->ptr[i]);
    }
    return llvm_ar_main2(arr);
#else
    return -1;
#endif
}

inline static chem::string_view allocate_view(ASTAllocator& allocator, const chem::string_view& v) {
    if(v.empty()) return "";
    return {allocator.allocate_str(v.data(), v.size())};
}

static std::span<chem::string_view> allocate_parts(ASTAllocator& allocator, const std::span<chem::string_view>& parts) {
    const auto ptr = (chem::string_view*) allocator.allocate_released_size(sizeof(chem::string_view) * parts.size(), alignof(chem::string_view));
    auto current = ptr;
    for(auto& sym : parts) {
        *current = allocate_view(allocator, sym);
        current++;
    }
    return {ptr, parts.size()};
}

static std::span<ImportSymbol> allocate_symbols(ASTAllocator& allocator, const std::span<RemoteImportSymbolCBI>& symbols) {
    const auto ptr = (ImportSymbol*) allocator.allocate_released_size(sizeof(ImportSymbol) * symbols.size(), alignof(ImportSymbol));
    auto current = ptr;
    for(auto& sym : symbols) {
        *current = ImportSymbol { allocate_parts(allocator, std::span(sym.parts.ptr, sym.parts.size)), allocate_view(allocator, sym.alias) };
        current++;
    }
    return {ptr, symbols.size()};
}

inline void add_remote_import(LabBuildContext* self, LabJob* job, RemoteImportCBI* dep, LabModule* mod) {
    auto& allocator = self->compiler.global_allocator;
    job->remote_imports.emplace_back(
            allocate_view(allocator, dep->from),
            allocate_view(allocator, dep->subdir),
            allocate_view(allocator, dep->version),
            allocate_view(allocator, dep->branch),
            allocate_view(allocator, dep->commit),
            DependencySymbolInfo{
                    allocate_symbols(allocator, std::span(dep->symbols.ptr, dep->symbols.size)),
                    allocate_view(allocator, dep->alias),
                    dep->location,
            },
            mod
    );
}

void BuildContextfetch_job_dependency(LabBuildContext* self, LabJob* job, RemoteImportCBI* dep) {
    add_remote_import(self, job, dep, nullptr);
}

void BuildContextfetch_mod_dependency(LabBuildContext* self, LabJob* job, LabModule* mod, RemoteImportCBI* dep) {
    add_remote_import(self, job, dep, mod);
}

static DependencySymbolInfo* allocate_dep_info(ASTAllocator& allocator, DependencySymbolInfoCBI* cbi_info) {
    if(!cbi_info) return nullptr;
    const auto info = (DependencySymbolInfo*) allocator.allocate_released_size(sizeof(DependencySymbolInfo), alignof(DependencySymbolInfo));
    info->symbols = allocate_symbols(allocator, std::span(cbi_info->symbols.ptr, cbi_info->symbols.size));
    info->alias = allocate_view(allocator, cbi_info->alias);
    info->location = cbi_info->location;
    return info;
}

LabModule* BuildContextnew_module_and_deps(LabBuildContext* self, chem::string_view* scope_name, chem::string_view* name, ModuleDependencyCBISpan* dependencies) {
    auto& allocator = self->compiler.global_allocator;
    auto mod = new LabModule(LabModuleType::Directory, chem::string(*scope_name), chem::string(*name));
    self->storage.insert_module_ptr_dangerous(mod);
    if(dependencies && dependencies->size > 0) {
        mod->reserve_dependencies(dependencies->size);
        for(size_t i = 0; i < dependencies->size; i++) {
            auto& dep = dependencies->ptr[i];
            mod->add_dependency(dep.module, allocate_dep_info(allocator, dep.info));
        }
    }
    return mod;
}

void BuildContextset_module_symbol_info(LabBuildContext* self, LabModule* module, unsigned int index, DependencySymbolInfoCBI* info) {
    if (index < module->dependencies.size()) {
        auto& dep = module->dependencies[index];
        dep.info = allocate_dep_info(self->compiler.global_allocator, info);
    }
}

