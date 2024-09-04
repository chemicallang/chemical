// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"
#include "utils/PathUtils.h"
#include <unordered_map>

LabBuildContext::LabBuildContext(LabBuildCompilerOptions* options, std::string lab_file, std::string user_build_dir) : options(options) {
    if(user_build_dir.empty()) {
        build_dir = resolve_non_canon_parent_path(lab_file, "build");
    } else {
        build_dir = user_build_dir;
    }
}

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

void LabBuildContext::add_paths(std::vector<chem::string>& into, chem::string** paths, unsigned int path_len) {
    if(!paths || path_len == 0) return;
    auto ptr = paths;
    unsigned i = 0;
    while (i < path_len) {
        into.emplace_back((*ptr)->copy());
        ptr++;
        i++;
    }
}

LabModule *LabBuildContext::add_with_type(
    LabModuleType type,
    chem::string name,
    chem::string** paths,
    unsigned int path_len,
    LabModule **dependencies,
    unsigned int dep_len
) {
    auto mod = new LabModule(type, std::move(name));
    modules.emplace_back(mod);
    LabBuildContext::add_paths(mod->paths, paths, path_len);
    LabBuildContext::add_dependencies(mod->dependencies, dependencies, dep_len);
    return mod;
}

LabModule* LabBuildContext::create_of_type(LabModuleType type, chem::string* path, unsigned number) {
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
    auto mod = add_with_type(type, chem::string(prefix + std::to_string(modules.size()) + '-' + std::to_string(number)), nullptr, 0, nullptr, 0);
    mod->paths.emplace_back(path->copy());
    return mod;
}

LabModule* LabBuildContext::files_module(
        chem::string* name,
        chem::string** paths,
        unsigned int path_len,
        LabModule** dependencies,
        unsigned int dep_len
) {
    // create a module with no files
    auto mod = add_with_type(LabModuleType::Files, name->copy(), nullptr, 0, dependencies, dep_len);
    if(paths && path_len != 0) {
        auto ptr = *paths;
        unsigned i = 0;
        while (i < path_len) {
            if(ptr->ends_with(".ch")) {
                mod->paths.emplace_back(ptr->copy());
            } else if(ptr->ends_with(".c")) {
                mod->dependencies.emplace_back(create_of_type(LabModuleType::CFile, ptr, i));
            } else if(ptr->ends_with(".o")) {
                mod->dependencies.emplace_back(create_of_type(LabModuleType::ObjFile, ptr, i));
            } else {

            }
            ptr++;
            i++;
        }
    }
    return mod;
}

LabJob* LabBuildContext::translate_to_chemical(
        chem::string* c_path,
        chem::string* out_path
) {
    auto job = new LabJob(LabJobType::ToChemicalTranslation, chem::string("ToChemicalJob"));
    executables.emplace_back(job);
    job->abs_path.append(out_path);
    auto reduntant_mod = new LabModule(LabModuleType::CFile, chem::string("CFile"));
    reduntant_mod->paths.emplace_back(c_path->copy());
    modules.emplace_back(reduntant_mod);
    job->dependencies.emplace_back(reduntant_mod);
    return job;
}

void LabBuildContext::set_build_dir(LabJob* job) {
    auto build_dir_path = resolve_rel_child_path_str(build_dir, job->name.to_std_string() + ".dir");
    job->build_dir.append(build_dir_path.data(), build_dir_path.size());
}

LabJob* LabBuildContext::translate_to_c(
        chem::string* name,
        LabModule** dependencies,
        unsigned int dep_len,
        chem::string* out_path
) {
    auto job = new LabJob(LabJobType::ToCTranslation, name->copy());
    executables.emplace_back(job);
    set_build_dir(job);
    job->abs_path.append(out_path);
    LabBuildContext::add_dependencies(job->dependencies, dependencies, dep_len);
    return job;
}

LabJob* LabBuildContext::build_exe(
        chem::string* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    auto exe = new LabJob(LabJobType::Executable, name->copy());
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto exe_path = resolve_rel_child_path_str(exe->build_dir.data(), name->to_std_string());
#ifdef _WINDOWS
    exe_path += ".exe";
#endif
    exe->abs_path.append(exe_path);
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}

LabJob* LabBuildContext::build_dynamic_lib(
        chem::string* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    auto exe = new LabJob(LabJobType::Library, name->copy());
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto output_path = resolve_rel_child_path_str(exe->build_dir.data(), name->to_std_string());
#ifdef _WIN32
        output_path += ".dll";
#elif __linux__
        output_path += ".so";
#else
        #error "Unknown operating system"
#endif
    exe->abs_path.append(output_path);
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}

bool LabBuildContext::has_arg(chem::string* name) {
    return build_args.find(name->to_std_string()) != build_args.end();
}

void LabBuildContext::get_arg(chem::string* str, chem::string* name) {
    auto found = build_args.find(name->to_std_string());
    if(found != build_args.end()) {
        str->append(found->second.data(), found->second.size());
    }
}

void LabBuildContext::remove_arg(chem::string* name) {
    auto found = build_args.find(name->to_std_string());
    if(found != build_args.end()) {
        build_args.erase(found);
    }
}