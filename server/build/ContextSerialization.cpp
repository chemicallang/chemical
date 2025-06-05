// Copyright (c) Chemical Language Foundation 2025.

#include "ContextSerialization.h"
#include "compiler/lab/LabBuildContext.h"
#include "lsp/json/json.h"
#include <lsp/enumeration.h>
#include <lsp/serialization.h>

// ---------------------------------------
// ---------------- toJson methods
// ---------------------------------------

lsp::json::Array dependencies_toJson(std::vector<LabModule*>& deps) {
    lsp::json::Array depArr;
    for(const auto dep : deps) {
        depArr.emplace_back(lsp::toJson(dep->format(':')));
    }
    return std::move(depArr);
}

lsp::json::Object labJob_toJson(LabJob* job) {
    lsp::json::Object jobObj;
    jobObj["type"] = lsp::toJson(static_cast<int>(job->type));
    jobObj["name"] = lsp::toJson(job->name.to_view());
    jobObj["dependencies"] = dependencies_toJson(job->dependencies);
    return std::move(jobObj);
}

lsp::json::Object labModule_toJson(LabModule* module) {
    lsp::json::Object modObj;
    modObj["type"] = lsp::toJson(static_cast<int>(module->type));
    modObj["scope_name"] = lsp::toJson(module->scope_name.to_view());
    modObj["name"] = lsp::toJson(module->name.to_view());

    lsp::json::Array pathsArr;
    for(auto& path : module->paths) {
        pathsArr.emplace_back(lsp::toJson(path.to_view()));
    }
    modObj["paths"] = std::move(pathsArr);

    modObj["dependencies"] = dependencies_toJson(module->dependencies);
    return std::move(modObj);
}

lsp::json::Object labBuildContext_toJson(BasicBuildContext& context) {
    lsp::json::Object contextObj;
    lsp::json::Array exeArr;
    for(auto& exe : context.executables) {
        exeArr.emplace_back(labJob_toJson(exe.get()));
    }
    lsp::json::Array modsArr;
    for(auto& mod : context.storage.get_modules()) {
        modsArr.emplace_back(labModule_toJson(mod.get()));
    }
    contextObj["executables"] = std::move(exeArr);
    contextObj["modules"] = std::move(modsArr);
    return std::move(contextObj);
}

std::string labBuildContext_toJsonStr(BasicBuildContext& context, bool format) {
    auto obj = labBuildContext_toJson(context);
    return lsp::json::stringify(obj, format);
}

//--------------------------------------
//------------fromJson methods----------
//--------------------------------------

void dependencies_fromJson(ModuleStorage& storage, lsp::json::Array depsArr, std::vector<LabModule*>& deps) {
    for(auto& dep : depsArr) {
        if(dep.isString()) {
            const auto found = storage.find_module(dep.string());
            if(found != nullptr) {
                deps.emplace_back(found);
            }
        }
    }
}

struct LabModuleDependencyRecord {
    LabModule* module;
    lsp::json::Array* depsArr;
};

LabModule* labModule_fromJson(lsp::json::Object& obj, std::vector<LabModuleDependencyRecord>& depsRec) {
    auto jobType = obj.find("type");
    if(jobType != obj.end() && jobType->second.isInteger()) {

        // the type of job
        auto labModType = static_cast<LabModuleType>(jobType->second.integer());

        // the scope name of the module
        auto scope_name = obj.find("scope_name");
        if(scope_name != obj.end() && scope_name->second.isString()) {

            auto scopeNameStr = scope_name->second.string();

            auto name = obj.find("name");
            if(name != obj.end() && name->second.isString()) {

                // the name of the module
                auto nameStr = name->second.string();

                const auto module = new LabModule(labModType, chem::string(scopeNameStr), chem::string(nameStr));

                // lets get paths
                auto paths = obj.find("paths");
                if(paths != obj.end() && paths->second.isArray()) {
                    auto& pathsArr = paths->second.array();
                    for(auto& path : pathsArr) {
                        if(path.isString()) {
                            module->paths.emplace_back(path.string());
                        }
                    }
                }

                // lets get dependencies
                auto deps = obj.find("dependencies");
                if(deps != obj.end() && deps->second.isArray()) {
                    auto& depsArr = deps->second.array();
                    depsRec.emplace_back(module, &depsArr);
                }

                return module;
            }

        }


    }
    return nullptr;
}

LabJob* labJob_fromJson(lsp::json::Object& obj, ModuleStorage& storage) {
    auto jobType = obj.find("type");
    if(jobType != obj.end() && jobType->second.isInteger()) {

        // the type of job
        auto labJobType = static_cast<LabJobType>(jobType->second.integer());

        auto name = obj.find("name");
        if(name != obj.end() && name->second.isString()) {

            // the name of the job
            auto nameStr = name->second.string();

            const auto job = new LabJob(labJobType, chem::string(nameStr));

            // lets get dependencies
            auto deps = obj.find("dependencies");
            if(deps != obj.end() && deps->second.isArray()) {
                auto& depsArr = deps->second.array();
                dependencies_fromJson(storage, depsArr, job->dependencies);
            }

            return job;
        }
    }
    return nullptr;
}

void labBuildContext_fromJson(BasicBuildContext& context, lsp::json::Object& obj) {
    std::vector<LabModuleDependencyRecord> depsRec;
    // we get all the modules, and index their dependencies as records
    auto modsArr = obj.find("modules");
    if(modsArr != obj.end() && modsArr->second.isArray()) {
        auto& arr = modsArr->second.array();
        for(auto& mod : arr) {
            if(mod.isObject()) {
                const auto modPtr = labModule_fromJson(mod.object(), depsRec);
                if(modPtr != nullptr) {
                    context.storage.insert_module_ptr_dangerous(modPtr);
                }
            }
        }
    }
    // now we find every dependency and wire it up
    for(auto& rec : depsRec) {
        auto& depsArr = *rec.depsArr;
        auto& modRef = *rec.module;
        for(auto& val : depsArr) {
            if(val.isString()) {
                const auto depFound = context.storage.find_module(val.string());
                if(depFound != nullptr) {
                    modRef.dependencies.emplace_back(depFound);
                }
            }
        }

    }
    // we get all the executables
    auto exeArr = obj.find("executables");
    if(exeArr != obj.end() && exeArr->second.isArray()) {
        auto& arr = exeArr->second.array();
        for(auto& exe : arr) {
            if(exe.isObject()) {
                const auto exePtr = labJob_fromJson(exe.object(), context.storage);
                if(exePtr != nullptr) {
                    context.executables.emplace_back(exePtr);
                }
            }
        }
    }
}

bool labBuildContext_fromJson(BasicBuildContext& context, std::string_view jsonContent) {
    auto thing = lsp::json::parse(jsonContent);
    if(thing.isObject()) {
        labBuildContext_fromJson(context, thing.object());
        return true;
    } else {
        return false;
    }
}