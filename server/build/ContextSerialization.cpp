// Copyright (c) Chemical Language Foundation 2025.

#include "ContextSerialization.h"
#include "compiler/lab/LabBuildContext.h"
#include "lsp/json/json.h"
#include <lsp/enumeration.h>
#include <lsp/serialization.h>
#include "compiler/cbi/model/CompilerBinder.h"

// ---------------------------------------
// ---------------- toJson methods
// ---------------------------------------

lsp::json::Object cbiFnIndex_toJson(CBIFunctionIndex& index) {
    lsp::json::Object obj;
    obj["fn_name"] = lsp::toJson(index.fn_name.to_view());
    obj["key"] = lsp::toJson(index.key.to_view());
    obj["type"] = lsp::toJson(static_cast<int>(index.fn_type));
    return std::move(obj);
}

lsp::json::Array dependencies_toJson(std::vector<LabModule*>& deps) {
    lsp::json::Array depArr;
    depArr.reserve(deps.size());
    for(const auto dep : deps) {
        depArr.emplace_back(lsp::toJson(dep->format(':')));
    }
    return std::move(depArr);
}

lsp::json::Array toJsonArray(std::vector<chem::string>& strArr) {
    lsp::json::Array jsonArr;
    jsonArr.reserve(strArr.size());
    for(auto& str : strArr) {
        jsonArr.emplace_back(lsp::toJson(str.to_view()));
    }
    return std::move(jsonArr);
}

lsp::json::Array toJsonArray(std::vector<CBIFunctionIndex>& indexes) {
    lsp::json::Array indexesArr;
    indexesArr.reserve(indexes.size());
    for(auto& fnIndex : indexes) {
        indexesArr.emplace_back(cbiFnIndex_toJson(fnIndex));
    }
    return std::move(indexesArr);
}

lsp::json::Object labJob_toJson(LabJob* job) {
    lsp::json::Object jobObj;
    jobObj["type"] = lsp::toJson(static_cast<int>(job->type));
    jobObj["name"] = lsp::toJson(job->name.to_view());
    // dependencies of the job
    if(!job->dependencies.empty()) {
        jobObj["dependencies"] = dependencies_toJson(job->dependencies);
    }
    // function indexes for cbi
    if(job->type == LabJobType::CBI) {
        const auto cbiJob = (LabJobCBI*) job;
        if(!cbiJob->indexes.empty()) {
            jobObj["indexes"] = toJsonArray(cbiJob->indexes);
        }
    }
    return std::move(jobObj);
}

lsp::json::Object labModule_toJson(LabModule* module) {
    lsp::json::Object modObj;
    modObj["type"] = lsp::toJson(static_cast<int>(module->type));
    modObj["scope_name"] = lsp::toJson(module->scope_name.to_view());
    modObj["name"] = lsp::toJson(module->name.to_view());

    // converting all the paths to a json array
    if(!module->paths.empty()) {
        modObj["paths"] = toJsonArray(module->paths);
    }

    // converting all compiler interfaces to a json array
    if(!module->compiler_interfaces_str.empty()) {
        modObj["interfaces"] = toJsonArray(module->compiler_interfaces_str);
    }

    // converting all the dependencies
    if(!module->dependencies.empty()) {
        modObj["dependencies"] = dependencies_toJson(module->dependencies);
    }

    return std::move(modObj);
}

lsp::json::Object labBuildContext_toJson(BasicBuildContext& context) {
    lsp::json::Object contextObj;
    lsp::json::Array exeArr;
    exeArr.reserve(context.executables.size());
    for(auto& exe : context.executables) {
        exeArr.emplace_back(labJob_toJson(exe.get()));
    }
    lsp::json::Array modsArr;
    modsArr.reserve(context.storage.get_modules().size());
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

void dependencies_fromJson(ModuleStorage& storage, const lsp::json::Array& depsArr, std::vector<LabModule*>& deps) {
    deps.reserve(depsArr.size());
    for(auto& dep : depsArr) {
        if(dep.isString()) {
            const auto found = storage.find_module(dep.string());
            if(found != nullptr) {
                deps.emplace_back(found);
            }
        }
    }
}

void indexes_fromJson(const lsp::json::Array& indexesArr, std::vector<CBIFunctionIndex>& fnIndexes) {
    fnIndexes.reserve(indexesArr.size());
    for(auto& indexObj : indexesArr) {
        if(!indexObj.isObject()) {
            continue;
        }
        const auto& index = indexObj.object();
        auto foundFnName = index.find("fn_name");
        auto foundKey = index.find("key");
        auto foundType = index.find("type");
        if(foundFnName == index.end() || foundKey == index.end() || foundType == index.end()) {
            continue;
        }
        auto& fnName = foundFnName->second;
        auto& key = foundKey->second;
        auto& type = foundType->second;
        if(!fnName.isString() || !key.isString() || !type.isInteger()) {
            continue;
        }
        fnIndexes.emplace_back(
            CBIFunctionIndex(
                    chem::string(key.string()),
                    chem::string(fnName.string()),
                    static_cast<CBIFunctionType>(type.integer())
            )
        );
    }
}

struct LabModuleDependencyRecord {
    LabModule* module;
    lsp::json::Array* depsArr;
};

LabModule* labModule_fromJson(
        lsp::json::Object& obj,
        std::vector<LabModuleDependencyRecord>& depsRec,
        CompilerBinder& binder
) {
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
                    module->paths.reserve(pathsArr.size());
                    for(auto& path : pathsArr) {
                        if(path.isString()) {
                            module->paths.emplace_back(path.string());
                        }
                    }
                }

                // lets get interfaces
                auto interfaces = obj.find("interfaces");
                if(interfaces != obj.end() && interfaces->second.isArray()) {
                    auto& ifsArr = interfaces->second.array();
                    module->compiler_interfaces.reserve(ifsArr.size());
                    for(auto& ifs : ifsArr) {
                        if(ifs.isString()) {
                            auto found = binder.interface_maps.find(chem::string_view(ifs.string()));
                            if(found != binder.interface_maps.end()) {
                                module->compiler_interfaces.emplace_back(found->second);
                            }
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

            // creating a lab job
            const auto job = labJobType == LabJobType::CBI ?
                    new LabJobCBI(chem::string(nameStr)) :
                    new LabJob(labJobType, chem::string(nameStr));

            // lets get dependencies
            auto deps = obj.find("dependencies");
            if(deps != obj.end() && deps->second.isArray()) {
                auto& depsArr = deps->second.array();
                dependencies_fromJson(storage, depsArr, job->dependencies);
            }

            // getting the indexes for cbi
            if(labJobType == LabJobType::CBI) {
                const auto cbiJob = (LabJobCBI*) job;
                auto indexes = obj.find("indexes");
                if(indexes != obj.end() && indexes->second.isArray()) {
                    indexes_fromJson(indexes->second.array(), cbiJob->indexes);
                }
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
                const auto modPtr = labModule_fromJson(mod.object(), depsRec, context.binder);
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