// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"
#include "utils/PathUtils.h"
#include <unordered_map>

LabBuildContext::LabBuildContext(std::string lab_file, std::string user_build_dir) {
    if(user_build_dir.empty()) {
        build_dir = resolve_non_canon_parent_path(lab_file, "build");
    } else {
        build_dir = user_build_dir;
    }
}

void LabBuildContext::add_dependencies(std::vector<LabModule*>& into, LabModule **dependencies, unsigned int dep_len) {
    if(!dependencies || dep_len == 0) return;
    auto ptr = *dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        into.emplace_back(ptr);
        ptr++;
        i++;
    }
}

void LabBuildContext::add_paths(std::vector<chem::string>& into, chem::string** paths, unsigned int path_len) {
    if(!paths || path_len == 0) return;
    auto ptr = *paths;
    unsigned i = 0;
    while (i < path_len) {
        into.emplace_back(ptr->copy());
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
    modules.emplace_back(type, std::move(name));
    auto mod = &modules.back();
    LabBuildContext::add_paths(mod->paths, paths, path_len);
    LabBuildContext::add_dependencies(mod->dependencies, dependencies, dep_len);
    return mod;
}

LabJob* LabBuildContext::build_exe(
        chem::string* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    executables.emplace_back(LabJobType::Executable, name->copy());
    auto exe = &executables.back();
    auto build_dir_path = resolve_rel_child_path_str(build_dir, name->to_std_string() + ".dir");
    exe->build_dir.append(build_dir_path.data(), build_dir_path.size());
    auto exe_path = resolve_sibling(build_dir_path, name->to_std_string());
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
    executables.emplace_back(LabJobType::Library, name->copy());
    auto exe = &executables.back();
    auto build_dir_path = resolve_rel_child_path_str(build_dir, name->to_std_string() + ".dir");
    exe->build_dir.append(build_dir_path.data(), build_dir_path.size());
    auto output_path = resolve_sibling(build_dir_path, name->to_std_string());
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