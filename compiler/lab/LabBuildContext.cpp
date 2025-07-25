// Copyright (c) Chemical Language Foundation 2025.

#include "LabBuildContext.h"
#include "preprocess/ImportPathHandler.h"
#include "utils/PathUtils.h"
#include <iostream>
#include <unordered_map>
#include <span>
#include "LabBuildCompiler.h"

void LabBuildContext::add_dependencies(std::vector<LabModule*>& into, LabModule **dependencies, unsigned int dep_len) {
    if(!dependencies || dep_len == 0) return;
    auto ptr = dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        into.emplace_back(*ptr);
        ptr++;
        i++;
    }
}

void LabBuildContext::add_dependencies(LabModule* module, LabModule** dependencies, unsigned int dep_len) {
    if(!dependencies || dep_len == 0) return;
    auto ptr = dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        module->add_dependency(*ptr);
        ptr++;
        i++;
    }
}

void LabBuildContext::add_paths(std::vector<chem::string>& into, chem::string_view** paths, unsigned int path_len) {
    if(!paths || path_len == 0) return;
    auto ptr = paths;
    unsigned i = 0;
    while (i < path_len) {
        into.emplace_back(**ptr);
        ptr++;
        i++;
    }
}

void LabBuildContext::declare_alias(std::unordered_map<std::string, std::string, StringHash, StringEqual>& aliases, std::string alias, std::string path) {
    const auto path_last = path.size() - 1;
    if(path[path_last] == '/') {
        path = path.substr(0, path_last);
    }
    aliases[std::move(alias)] = std::move(path);
}

bool LabBuildContext::declare_user_alias(LabJob* job, std::string alias, std::string path) {
    auto found = job->path_aliases.find(alias);
    if(found != job->path_aliases.end()) {
        std::cerr << "[lab] error declaring alias '" << alias << "' for path '" << path << "', an alias with same already exists in job '" << job->name << "'" << std::endl;
        return false;
    }
    while(path[0] == '@') {
        auto result = handler.replace_at_in_path(path, job->path_aliases);
        if(result.error.empty()) {
            path = std::move(result.replaced);
        } else {
            std::cerr << "[lab] error declaring alias '" << alias << "' for path '" << path << "', " << result.error << " in job '" << job->name << "'" << std::endl;
            return false;
        }
    }
    declare_alias(job->path_aliases, std::move(alias), std::move(path));
    return true;
}

void LabBuildContext::put_path_aliases(LabJob* job, LabModule* module) {
    if(module->type == LabModuleType::Directory) {
        declare_alias(job->path_aliases, module->name.to_std_string(), module->paths[0].to_std_string());
    }
    for(auto dep : module->get_dependencies()) {
        put_path_aliases(job, dep);
    }
}

void LabBuildContext::init_path_aliases(LabJob* job) {
    for(auto dep : job->dependencies) {
        put_path_aliases(job, dep);
    }
}

LabModule* LabBuildContext::module_from_directory(const chem::string_view& path, const chem::string_view& scope_name, const chem::string_view& mod_name, chem::string& error_msg) {
    // keeping it here
    ModuleDependencyRecord dep(path.str());
    // TODO we cannot get the error yet
    // instead of returning error, create_module_for_dependency just prints it to stdcerr
    return compiler.create_module_for_dependency(*this, dep);
}

LabModule* LabBuildContext::add_with_type(
        LabModuleType type,
        const chem::string_view& scope_name,
        const chem::string_view& module_name,
        chem::string_view** paths,
        unsigned int path_len,
        LabModule** dependencies,
        unsigned int dep_len
) {
    const auto found_module = storage.find_module(scope_name, module_name);
    if(found_module != nullptr) {
        return found_module;
    }
    auto mod = new LabModule(type, chem::string(scope_name), chem::string(module_name));
    storage.insert_module_ptr_dangerous(mod);
    LabBuildContext::add_paths(mod->paths, paths, path_len);
    LabBuildContext::add_dependencies(mod, dependencies, dep_len);
    return mod;
}

LabModule* LabBuildContext::create_of_type(LabModuleType type, chem::string_view* path, unsigned number) {
    const char* prefix;
    switch(type) {
        case LabModuleType::CFile:
            prefix = "CFile-";
            break;
        case LabModuleType::ObjFile:
            prefix = "ObjFile-";
            break;
        default:
            prefix = "UnkFile-";
            break;
    }
    chem::string mod_name;
    mod_name.append(prefix);
    mod_name.append(std::to_string(storage.modules_size()));
    mod_name.append('-');
    mod_name.append(std::to_string(number));
    // TODO scope_name here is empty
    auto mod = add_with_type(type, "", mod_name.to_chem_view(), nullptr, 0, nullptr, 0);
    mod->paths.emplace_back(*path);
    return mod;
}

LabModule* LabBuildContext::files_module(
        const chem::string_view& scope_name,
        const chem::string_view& module_name,
        chem::string_view** paths,
        unsigned int path_len,
        LabModule** dependencies,
        unsigned int dep_len
) {
    // create a module with no files
    auto mod = add_with_type(LabModuleType::Directory, scope_name, module_name, nullptr, 0, dependencies, dep_len);
    if(paths && path_len != 0) {
        auto ptr = *paths;
        unsigned i = 0;
        while (i < path_len) {
            if(ptr->ends_with(".c")) {
                mod->add_dependency(create_of_type(LabModuleType::CFile, ptr, i));
            } else if(ptr->ends_with(".o")) {
                mod->add_dependency(create_of_type(LabModuleType::ObjFile, ptr, i));
            } else {
                mod->paths.emplace_back(*ptr);
            }
            ptr++;
            i++;
        }
    }
    return mod;
}

LabJob* LabBuildContext::translate_to_chemical(
        LabModule* module,
        chem::string_view* out_path
) {
    auto job = new LabJob(LabJobType::ToChemicalTranslation, chem::string("ToChemicalJob"));
    executables.emplace_back(job);
    job->abs_path.append(*out_path);
    job->dependencies.emplace_back(module);
    return job;
}

void LabBuildContext::set_build_dir(LabJob* job) {
    auto build_dir_path = resolve_rel_child_path_str(compiler.options->build_dir, job->name.to_std_string() + ".dir");
    job->build_dir.append(build_dir_path.data(), build_dir_path.size());
}

LabJob* LabBuildContext::translate_to_c(
        chem::string_view* name,
        LabModule** dependencies,
        unsigned int dep_len,
        chem::string_view* out_path
) {
    auto job = new LabJob(LabJobType::ToCTranslation, chem::string(*name));
    executables.emplace_back(job);
    set_build_dir(job);
    job->abs_path.append(*out_path);
    LabBuildContext::add_dependencies(job->dependencies, dependencies, dep_len);
    return job;
}

LabJob* LabBuildContext::build_exe(
        chem::string_view* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    auto exe = new LabJob(LabJobType::Executable, chem::string(*name));
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto exe_path = resolve_rel_child_path_str(exe->build_dir.to_view(), name->view());
#ifdef _WINDOWS
    exe_path += ".exe";
#endif
    exe->abs_path.append(exe_path);
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}

LabJob* LabBuildContext::build_dynamic_lib(
        chem::string_view* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    auto exe = new LabJob(LabJobType::Library, chem::string(*name));
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto output_path = resolve_rel_child_path_str(exe->build_dir.to_view(), name->view());
#ifdef _WIN32
        output_path += ".dll";
#elif defined(__APPLE__)
        output_path += ".dylib";
#else
        output_path += ".so";
#endif
    exe->abs_path.append(output_path);
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}

LabJob* LabBuildContext::build_cbi(
        chem::string_view* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    auto exe = new LabJobCBI(chem::string(*name));
    executables.emplace_back(exe);
    set_build_dir(exe);
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}

bool LabBuildContext::has_arg(const std::string& name) {
    return build_args.find(name) != build_args.end();
}

void LabBuildContext::get_arg(chem::string& str, const std::string& name) {
    auto found = build_args.find(name);
    if(found != build_args.end()) {
        str.append(found->second.data(), found->second.size());
    }
}

void LabBuildContext::remove_arg(const std::string& name) {
    auto found = build_args.find(name);
    if(found != build_args.end()) {
        build_args.erase(found);
    }
}