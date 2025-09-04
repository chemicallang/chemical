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

void set_job_mode(TargetData& data, OutputMode mode) {
    data.debug = is_debug(mode);
    data.debug_quick = mode == OutputMode::DebugQuick;
    data.debug_complete = mode == OutputMode::DebugComplete;
    data.release = is_release(mode);
    data.release_safe = mode == OutputMode::ReleaseSafe;
    data.release_small = mode == OutputMode::ReleaseSmall;
    data.release_fast = mode == OutputMode::ReleaseFast;
}

void initialize_job(LabJob* job, LabBuildCompilerOptions* options, const std::string& target_triple) {
    // setting if compiler is tcc
    if(options->use_tcc) {
        job->target_data.tcc = true;
    } else {
        switch(job->type) {
            case LabJobType::CBI:
                // no need to initialize target triple
                // keeping it empty, so host target triple is used
                job->target_data.tcc = true;
                job->target_data.cbi = true;
                break;
            case LabJobType::JITExecutable:
                // no need to initialize target triple
                // keeping it empty, so host target triple is used
                job->target_data.tcc = true;
                break;
            // why is this commented
            // when translating to C, we do not target tcc
            // user has to explicitly provide --use-tcc to make that happen
            // case LabJobType::ToCTranslation:
            default:
                job->target_data.tcc = false;
                break;
        }
    }
    // tiny cc doesn't support target triple
    job->target_triple.clear();
    if(!target_triple.empty()) {
        job->target_triple.append(target_triple);
    }
    // setting the default output mode
    job->mode = options->outMode;
    set_job_mode(job->target_data, options->outMode);
    // setting job specific variables
#ifdef LSP_BUILD
    job->target_data.lsp = true;
#else
    job->target_data.lsp = false;
#endif
    // we should allow changing this
    job->target_data.test = false;

}

void initialize_job(LabJob* job, LabBuildCompilerOptions* options) {
    initialize_job(job, options, options->target_triple);
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

LabModule* LabBuildContext::add_with_type(
        LabModuleType type,
        const chem::string_view& scope_name,
        const chem::string_view& module_name,
        chem::string_view** paths,
        unsigned int path_len,
        LabModule** dependencies,
        unsigned int dep_len
) {
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

void LabBuildContext::put_job_before(LabJob* newJob, LabJob* existingJob) {
    // lets first remove the job (scanning backwards)
    auto& v = executables;
    if (newJob == existingJob) return; // nothing to do
    int newIdx = -1; // index of the newJob we found (the one to potentially move)
    // single pass from back to front
    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i) {
        if (v[i].get() == existingJob) {
            if (newIdx == -1) {
                // we found existingJob before any newJob -> do nothing
                return;
            } else {
                // found existingInt and we have a candidate newInt at newIdx
                auto val = v[newIdx].release();
                v.erase(v.begin() + newIdx);   // remove the saved newInt
                // newIdx > i by construction (we found newInt while scanning from the back),
                // so erasing at newIdx does not change index i.
                v.insert(v.begin() + i, std::unique_ptr<LabJob>(val));  // put it before existingInt at index i
                return;
            }
        }
        // record the first newInt we encounter (closest to the back)
        if (newIdx == -1 && v[i].get() == newJob) {
            newIdx = i;
        }
    }
    // finished loop: either we never saw newJob, or we saw it but never found existingJob afterward.
}

LabJob* LabBuildContext::translate_to_chemical(
        LabModule* module,
        chem::string_view* out_path
) {
    auto job = new LabJob(LabJobType::ToChemicalTranslation, chem::string("ToChemicalJob"));
    initialize_job(job, compiler.options);
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
        chem::string_view* out_path
) {
    auto job = new LabJob(LabJobType::ToCTranslation, chem::string(*name));
    initialize_job(job, compiler.options);
    executables.emplace_back(job);
    set_build_dir(job);
    job->abs_path.append(*out_path);
    return job;
}

LabJob* LabBuildContext::build_exe(
        chem::string_view* name
) {
    auto exe = new LabJob(LabJobType::Executable, chem::string(*name));
    initialize_job(exe, compiler.options);
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto exe_path = resolve_rel_child_path_str(exe->build_dir.to_view(), name->view());
#ifdef _WINDOWS
    exe_path += ".exe";
#endif
    exe->abs_path.append(exe_path);
    return exe;
}

LabJob* LabBuildContext::run_jit_exe(
        chem::string_view* name
) {
    auto exe = new LabJob(LabJobType::JITExecutable, chem::string(*name));
    initialize_job(exe, compiler.options);
    executables.emplace_back(exe);
    set_build_dir(exe);
    auto exe_path = resolve_rel_child_path_str(exe->build_dir.to_view(), name->view());
#ifdef _WINDOWS
    exe_path += ".exe";
#endif
    exe->abs_path.append(exe_path);
    return exe;
}

LabJob* LabBuildContext::build_dynamic_lib(
        chem::string_view* name
) {
    auto exe = new LabJob(LabJobType::Library, chem::string(*name));
    initialize_job(exe, compiler.options);
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
    return exe;
}

LabJob* LabBuildContext::build_cbi(
        chem::string_view* name
) {
    auto exe = new LabJobCBI(chem::string(*name));
    initialize_job((LabJob*) exe, compiler.options);
    executables.emplace_back(exe);
    set_build_dir(exe);
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