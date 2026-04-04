// Copyright (c) Chemical Language Foundation 2025.

#include "ContextSerialization.h"
#include "lsp/json/json.h"
#include <lsp/enumeration.h>
#include <lsp/serialization.h>

#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/lab/ModuleStorage.h"
#include "compiler/lab/LabJob.h"
#include "compiler/lab/LabModule.h"
#include "core/source/SourceLocation.h"

void BuildContextInformation::clear() {
    modStorage.clear();
    jobs.clear();
    allocator.clear();
    symbol_info_pool.clear();
    symbol_lists_pool.clear();
    parts_lists_pool.clear();
    id_to_module.clear();
}

chem::string_view BuildContextInformation::pool_string(const std::string& str) {
    const auto allocated = allocator.allocate_str(str.data(), str.size());
    return chem::string_view(allocated, str.size());
}

std::string BuildContextInformation::built_cbi_map_to_str() {
    std::string built_cbi_str;
    auto& cbiMap = binder.get_cbi_map();
    auto total_size = cbiMap.size(); // total commas
    for (auto& kv : cbiMap) {
        total_size += kv.first.size();
    }
    built_cbi_str.reserve(total_size);
    for (auto& kv : cbiMap) {
        built_cbi_str.append(kv.first);
        built_cbi_str.append(1, ',');
    }
    // omit the last comma
    if (!built_cbi_str.empty()) {
        built_cbi_str.resize(built_cbi_str.size() - 1);
    }
    return std::move(built_cbi_str);
}

lsp::json::Value toJson(chem::string_view view) {
    return lsp::toJson(view.view());
}

// ---------------------------------------
// ---------------- toJson methods
// ---------------------------------------

lsp::json::Object cbiFnIndex_toJson(const CBIFunctionIndex& index) {
    lsp::json::Object obj;
    obj["fn_name"] = lsp::toJson(index.fn_name.to_view());
    obj["key"] = lsp::toJson(index.key.to_view());
    obj["type"] = lsp::toJson(static_cast<int>(index.fn_type));
    return std::move(obj);
}

lsp::json::Object importSymbol_toJson(const ImportSymbol& symbol) {
    lsp::json::Object obj;
    lsp::json::Array partsArr;
    for (auto part : symbol.parts) {
        partsArr.emplace_back(toJson(part));
    }
    obj["parts"] = std::move(partsArr);
    obj["alias"] = toJson(symbol.alias);
    return std::move(obj);
}

lsp::json::Object dependencySymbolInfo_toJson(const DependencySymbolInfo* info) {
    lsp::json::Object obj;
    if (info) {
        lsp::json::Array symbolsArr;
        for (const auto& sym : info->symbols) {
            symbolsArr.emplace_back(importSymbol_toJson(sym));
        }
        obj["symbols"] = std::move(symbolsArr);
        obj["alias"] = toJson(info->alias);
        obj["location"] = lsp::toJson(info->location.encoded);
    }
    return std::move(obj);
}

lsp::json::Object dependency_toJson(const ModuleDependency& dep, const std::unordered_map<LabModule*, int>& modToId) {
    lsp::json::Object obj;
    if (dep.module) {
        auto it = modToId.find(dep.module);
        if (it != modToId.end()) {
            obj["module_id"] = lsp::toJson(it->second);
        }
    }
    if (dep.info) {
        obj["info"] = dependencySymbolInfo_toJson(dep.info);
    }
    return std::move(obj);
}

lsp::json::Array dependencies_toJson(const std::vector<ModuleDependency>& deps, const std::unordered_map<LabModule*, int>& modToId) {
    lsp::json::Array depArr;
    depArr.reserve(deps.size());
    for (const auto& dep : deps) {
        depArr.emplace_back(dependency_toJson(dep, modToId));
    }
    return std::move(depArr);
}

lsp::json::Object remoteImport_toJson(const RemoteImport* ri, const std::unordered_map<LabModule*, int>& modToId) {
    lsp::json::Object obj;
    obj["from"] = toJson(ri->from);
    obj["subdir"] = toJson(ri->subdir);
    obj["version"] = toJson(ri->version);
    obj["branch"] = toJson(ri->branch);
    obj["commit"] = toJson(ri->commit);
    obj["mod_scope"] = toJson(ri->mod_scope);
    obj["mod_name"] = toJson(ri->mod_name);
    obj["origin"] = toJson(ri->origin);
    obj["orphan_branch"] = lsp::toJson(ri->orphan_branch);

    if (ri->built_module) {
        auto it = modToId.find(ri->built_module);
        if (it != modToId.end()) {
            obj["built_module_id"] = lsp::toJson(it->second);
        }
    }

    lsp::json::Array reqArr;
    for (const auto& req : ri->requesters) {
        lsp::json::Object reqObj;
        if (req.requester) {
            auto it = modToId.find(req.requester);
            if (it != modToId.end()) {
                reqObj["requester_id"] = lsp::toJson(it->second);
            }
        }
        if (req.symbol_info) {
            reqObj["symbol_info"] = dependencySymbolInfo_toJson(req.symbol_info);
        }
        reqArr.emplace_back(std::move(reqObj));
    }
    obj["requesters"] = std::move(reqArr);
    return std::move(obj);
}

lsp::json::Object definitions_toJson(const std::unordered_map<std::string, bool>& defs) {
    lsp::json::Object obj;
    for (const auto& [key, val] : defs) {
        obj[key] = lsp::toJson(val);
    }
    return std::move(obj);
}

lsp::json::Array toJsonArray(const std::vector<chem::string>& strArr) {
    lsp::json::Array jsonArr;
    jsonArr.reserve(strArr.size());
    for (const auto& str : strArr) {
        jsonArr.emplace_back(lsp::toJson(str.to_view()));
    }
    return std::move(jsonArr);
}

lsp::json::Array toJsonArray(const std::vector<CBIFunctionIndex>& indexes) {
    lsp::json::Array indexesArr;
    indexesArr.reserve(indexes.size());
    for (const auto& fnIndex : indexes) {
        indexesArr.emplace_back(cbiFnIndex_toJson(fnIndex));
    }
    return std::move(indexesArr);
}

lsp::json::Object labJob_toJson(LabJob* job, const std::unordered_map<LabModule*, int>& modToId) {
    lsp::json::Object obj;
    obj["type"] = lsp::toJson(static_cast<int>(job->type));
    obj["name"] = lsp::toJson(job->name.to_view());
    obj["abs_path"] = lsp::toJson(job->abs_path.to_view());
    obj["build_dir"] = lsp::toJson(job->build_dir.to_view());
    obj["status"] = lsp::toJson(static_cast<int>(job->status));
    obj["optional_job"] = lsp::toJson(job->optional_job);
    obj["target_triple"] = lsp::toJson(job->target_triple.to_view());
    obj["mode"] = lsp::toJson(static_cast<int>(job->mode));

    if (!job->objects.empty()) {
        obj["objects"] = toJsonArray(job->objects);
    }
    if (!job->link_libs.empty()) {
        obj["link_libs"] = toJsonArray(job->link_libs);
    }
    if (!job->lib_search_paths.empty()) {
        obj["lib_search_paths"] = toJsonArray(job->lib_search_paths);
    }
    if (!job->ship_files.empty()) {
        obj["ship_files"] = toJsonArray(job->ship_files);
    }
    if (!job->dependencies.empty()) {
        obj["dependencies"] = dependencies_toJson(job->dependencies, modToId);
    }

    if (!job->remote_imports.empty()) {
        lsp::json::Array riArr;
        for (const auto& ri : job->remote_imports) {
            riArr.emplace_back(remoteImport_toJson(ri.get(), modToId));
        }
        obj["remote_imports"] = std::move(riArr);
    }

    if (!job->definitions.empty()) {
        obj["definitions"] = definitions_toJson(job->definitions);
    }
    obj["conflict_strategy"] = lsp::toJson(static_cast<int>(job->conflict_strategy));

    if (job->type == LabJobType::CBI) {
        const auto cbiJob = (LabJobCBI*) job;
        if (!cbiJob->indexes.empty()) {
            obj["indexes"] = toJsonArray(cbiJob->indexes);
        }
    }
    return std::move(obj);
}

lsp::json::Object labModule_toJson(LabModule* module, const std::unordered_map<LabModule*, int>& modToId) {
    lsp::json::Object obj;
    obj["type"] = lsp::toJson(static_cast<int>(module->type));
    obj["package_kind"] = lsp::toJson(static_cast<int>(module->package_kind));
    obj["scope_name"] = lsp::toJson(module->scope_name.to_view());
    obj["name"] = lsp::toJson(module->name.to_view());

    if (!module->include_dirs.empty()) {
        obj["include_dirs"] = toJsonArray(module->include_dirs);
    }
    if (!module->paths.empty()) {
        obj["paths"] = toJsonArray(module->paths);
    }

    if (!module->compiler_interfaces_str.empty()) {
        obj["interfaces"] = toJsonArray(module->compiler_interfaces_str);
    }

    if (!module->dependencies.empty()) {
        obj["dependencies"] = dependencies_toJson(module->dependencies, modToId);
    }

    return std::move(obj);
}

lsp::json::Object labBuildContext_toJson(BuildContextInformation& context) {
    lsp::json::Object contextObj;

    std::unordered_map<LabModule*, int> modToId;
    const auto& modules = context.modStorage.get_modules();
    for (int i = 0; i < modules.size(); ++i) {
        modToId[modules[i].get()] = i;
    }

    lsp::json::Array modsArr;
    modsArr.reserve(modules.size());
    for (auto& mod : modules) {
        modsArr.emplace_back(labModule_toJson(mod.get(), modToId));
    }

    lsp::json::Array exeArr;
    exeArr.reserve(context.jobs.size());
    for (auto& exe : context.jobs) {
        exeArr.emplace_back(labJob_toJson(exe.get(), modToId));
    }

    contextObj["executables"] = std::move(exeArr);
    contextObj["modules"] = std::move(modsArr);
    return std::move(contextObj);
}

std::string labBuildContext_toJsonStr(BuildContextInformation& context, bool format) {
    return lsp::json::stringify(labBuildContext_toJson(context), format);
}

//--------------------------------------
//------------fromJson methods----------
//--------------------------------------

LabModule* findModuleById(BuildContextInformation& context, const lsp::json::Value* val) {
    if (val && val->isInteger()) {
        int id = val->integer();
        if (id >= 0 && id < context.id_to_module.size()) {
            return context.id_to_module[id];
        }
    }
    return nullptr;
}

ImportSymbol importSymbol_fromJson(const lsp::json::Object& obj, BuildContextInformation& context) {
    ImportSymbol sym;
    const auto parts = obj.find("parts");
    if (parts && parts->isArray()) {
        auto& arr = parts->array();
        std::vector<chem::string_view> partsVec;
        partsVec.reserve(arr.size());
        for (auto& p : arr) {
            if (p.isString()) {
                partsVec.emplace_back(context.pool_string(p.string()));
            }
        }
        context.parts_lists_pool.emplace_back(std::move(partsVec));
        sym.parts = context.parts_lists_pool.back();
    }
    const auto alias = obj.find("alias");
    if (alias && alias->isString()) {
        sym.alias = context.pool_string(alias->string());
    }
    return sym;
}

DependencySymbolInfo* dependencySymbolInfo_fromJson(const lsp::json::Object& obj, BuildContextInformation& context) {
    auto info = std::make_unique<DependencySymbolInfo>(std::span<ImportSymbol>(), "", 0);
    const auto symbols = obj.find("symbols");
    if (symbols && symbols->isArray()) {
        auto& arr = symbols->array();
        std::vector<ImportSymbol> symList;
        symList.reserve(arr.size());
        for (auto& s : arr) {
            if (s.isObject()) {
                symList.emplace_back(importSymbol_fromJson(s.object(), context));
            }
        }
        context.symbol_lists_pool.emplace_back(std::move(symList));
        info->symbols = context.symbol_lists_pool.back();
    }
    const auto alias = obj.find("alias");
    if (alias && alias->isString()) {
        info->alias = context.pool_string(alias->string());
    }
    const auto location = obj.find("location");
    if (location && location->isInteger()) {
        info->location = SourceLocation(location->integer());
    }

    context.symbol_info_pool.emplace_back(std::move(info));
    return context.symbol_info_pool.back().get();
}

void indexes_fromJson(const lsp::json::Array& indexesArr, std::vector<CBIFunctionIndex>& fnIndexes) {
    fnIndexes.reserve(indexesArr.size());
    for (auto& indexObj : indexesArr) {
        if (!indexObj.isObject()) {
            continue;
        }
        const auto& index = indexObj.object();
        auto foundFnName = index.find("fn_name");
        auto foundKey = index.find("key");
        auto foundType = index.find("type");
        if (foundFnName == nullptr || foundKey == nullptr || foundType == nullptr) {
            continue;
        }
        if (!foundFnName->isString() || !foundKey->isString() || !foundType->isInteger()) {
            continue;
        }
        fnIndexes.emplace_back(
            CBIFunctionIndex(
                    chem::string(foundKey->string()),
                    chem::string(foundFnName->string()),
                    static_cast<CBIFunctionType>(foundType->integer())
            )
        );
    }
}

std::unique_ptr<RemoteImport> remoteImport_fromJson(const lsp::json::Object& obj, BuildContextInformation& context) {
    auto ri = std::make_unique<RemoteImport>();
    if (auto f = obj.find("from")) {
        if (f->isString()) {
            ri->from = context.pool_string(f->string());
        }
    }
    if (auto s = obj.find("subdir")) {
        if (s->isString()) {
            ri->subdir = context.pool_string(s->string());
        }
    }
    if (auto v = obj.find("version")) {
        if (v->isString()) {
            ri->version = context.pool_string(v->string());
        }
    }
    if (auto b = obj.find("branch")) {
        if (b->isString()) {
            ri->branch = context.pool_string(b->string());
        }
    }
    if (auto c = obj.find("commit")) {
        if (c->isString()) {
            ri->commit = context.pool_string(c->string());
        }
    }
    if (auto ms = obj.find("mod_scope")) {
        if (ms->isString()) {
            ri->mod_scope = context.pool_string(ms->string());
        }
    }
    if (auto mn = obj.find("mod_name")) {
        if (mn->isString()) {
            ri->mod_name = context.pool_string(mn->string());
        }
    }
    if (auto o = obj.find("origin")) {
        if (o->isString()) {
            ri->origin = context.pool_string(o->string());
        }
    }
    if (auto ob = obj.find("orphan_branch")) {
        if (ob->isBoolean()) {
            ri->orphan_branch = ob->boolean();
        }
    }

    if (auto bm = obj.find("built_module_id")) {
        ri->built_module = findModuleById(context, bm);
    }

    if (auto reqs = obj.find("requesters")) {
        if (reqs->isArray()) {
            for (auto& rVal : reqs->array()) {
                if (rVal.isObject()) {
                    auto& rObj = rVal.object();
                    RemoteImportRequester req;
                    req.requester = nullptr;
                    if (auto rr = rObj.find("requester_id")) {
                        req.requester = findModuleById(context, rr);
                    }
                    req.symbol_info = nullptr;
                    if (auto si = rObj.find("symbol_info")) {
                        if (si->isObject()) {
                            req.symbol_info = dependencySymbolInfo_fromJson(si->object(), context);
                        }
                    }
                    ri->requesters.emplace_back(req);
                }
            }
        }
    }
    return std::move(ri);
}

struct LabModuleDependencyRecord {
    LabModule* module;
    const lsp::json::Array* depsArr;
};

LabModule* labModule_fromJson(
        const lsp::json::Object& obj,
        std::vector<LabModuleDependencyRecord>& depsRec,
        CompilerBinder& binder
) {
    auto typeFound = obj.find("type");
    auto scopeFound = obj.find("scope_name");
    auto nameFound = obj.find("name");

    if (typeFound && typeFound->isInteger() && scopeFound && scopeFound->isString() && nameFound && nameFound->isString()) {
        const auto module = new LabModule(
            static_cast<LabModuleType>(typeFound->integer()),
            chem::string(scopeFound->string()),
            chem::string(nameFound->string())
        );

        if (auto pk = obj.find("package_kind")) {
            if (pk->isInteger()) {
                module->package_kind = static_cast<PackageKind>(pk->integer());
            }
        }

        if (auto paths = obj.find("paths")) {
            if (paths->isArray()) {
                for (auto& p : paths->array()) {
                    if (p.isString()) {
                        module->paths.emplace_back(p.string());
                    }
                }
            }
        }

        if (auto id = obj.find("include_dirs")) {
            if (id->isArray()) {
                for (auto& p : id->array()) {
                    if (p.isString()) {
                        module->include_dirs.emplace_back(p.string());
                    }
                }
            }
        }

        if (auto interfaces = obj.find("interfaces")) {
            if (interfaces->isArray()) {
                for (auto& ifs : interfaces->array()) {
                    if (ifs.isString()) {
                        auto found = binder.interface_maps.find(chem::string_view(ifs.string()));
                        if (found != binder.interface_maps.end()) {
                            module->compiler_interfaces.emplace_back(found->second);
                        }
                        module->compiler_interfaces_str.emplace_back(ifs.string());
                    }
                }
            }
        }

        if (auto deps = obj.find("dependencies")) {
            if (deps->isArray()) {
                depsRec.emplace_back(module, &deps->array());
            }
        }

        return module;
    }
    return nullptr;
}

LabJob* labJob_fromJson(const lsp::json::Object& obj, BuildContextInformation& context) {
    auto typeFound = obj.find("type");
    auto nameFound = obj.find("name");

    if (typeFound && typeFound->isInteger() && nameFound && nameFound->isString()) {
        auto labJobType = static_cast<LabJobType>(typeFound->integer());
        const auto job = labJobType == LabJobType::CBI ?
                new LabJobCBI(chem::string(nameFound->string()), OutputMode::Debug) :
                new LabJob(labJobType, chem::string(nameFound->string()), OutputMode::Debug);

        if (auto ap = obj.find("abs_path")) {
            if (ap->isString()) {
                job->abs_path = chem::string(ap->string());
            }
        }
        if (auto bd = obj.find("build_dir")) {
            if (bd->isString()) {
                job->build_dir = chem::string(bd->string());
            }
        }
        if (auto st = obj.find("status")) {
            if (st->isInteger()) {
                job->status = static_cast<LabJobStatus>(st->integer());
            }
        }
        if (auto oj = obj.find("optional_job")) {
            if (oj->isBoolean()) {
                job->optional_job = oj->boolean();
            }
        }
        if (auto tt = obj.find("target_triple")) {
            if (tt->isString()) {
                job->target_triple = chem::string(tt->string());
            }
        }
        if (auto md = obj.find("mode")) {
            if (md->isInteger()) {
                job->mode = static_cast<OutputMode>(md->integer());
            }
        }

        if (auto ob = obj.find("objects")) {
            if (ob->isArray()) {
                for (auto& p : ob->array()) {
                    if (p.isString()) {
                        job->objects.emplace_back(p.string());
                    }
                }
            }
        }
        if (auto ll = obj.find("link_libs")) {
            if (ll->isArray()) {
                for (auto& p : ll->array()) {
                    if (p.isString()) {
                        job->link_libs.emplace_back(p.string());
                    }
                }
            }
        }
        if (auto sp = obj.find("lib_search_paths")) {
            if (sp->isArray()) {
                for (auto& p : sp->array()) {
                    if (p.isString()) {
                        job->lib_search_paths.emplace_back(p.string());
                    }
                }
            }
        }
        if (auto sf = obj.find("ship_files")) {
            if (sf->isArray()) {
                for (auto& p : sf->array()) {
                    if (p.isString()) {
                        job->ship_files.emplace_back(p.string());
                    }
                }
            }
        }

        if (auto deps = obj.find("dependencies")) {
            if (deps->isArray()) {
                for (auto& val : deps->array()) {
                    if (val.isObject()) {
                        const auto& depObj = val.object();
                        const auto modIdVal = depObj.find("module_id");
                        const auto depFound = findModuleById(context, modIdVal);
                        if (depFound) {
                            DependencySymbolInfo* info = nullptr;
                            const auto infoObj = depObj.find("info");
                            if (infoObj && infoObj->isObject()) {
                                info = dependencySymbolInfo_fromJson(infoObj->object(), context);
                            }
                            job->add_dependency(depFound, info);
                        }
                    }
                }
            }
        }

        if (auto riArr = obj.find("remote_imports")) {
            if (riArr->isArray()) {
                for (auto& riVal : riArr->array()) {
                    if (riVal.isObject()) {
                        job->remote_imports.emplace_back(remoteImport_fromJson(riVal.object(), context));
                    }
                }
            }
        }

        if (auto defs = obj.find("definitions")) {
            if (defs->isObject()) {
                for (auto& [k, v] : defs->object().keyValueMap()) {
                    if (v.isBoolean()) {
                        job->definitions[k] = v.boolean();
                    }
                }
            }
        }

        if (auto cs = obj.find("conflict_strategy")) {
            if (cs->isInteger()) {
                job->conflict_strategy = static_cast<ConflictResolutionStrategy>(cs->integer());
            }
        }

        if (labJobType == LabJobType::CBI) {
            const auto cbiJob = (LabJobCBI*) job;
            if (auto indexes = obj.find("indexes")) {
                if (indexes->isArray()) {
                    indexes_fromJson(indexes->array(), cbiJob->indexes);
                }
            }
        }

        return job;
    }
    return nullptr;
}

void labBuildContext_fromJson(BuildContextInformation& context, lsp::json::Object& obj) {
    std::vector<LabModuleDependencyRecord> depsRec;
    if (auto modsArr = obj.find("modules")) {
        if (modsArr->isArray()) {
            const auto& arr = modsArr->array();
            context.id_to_module.resize(arr.size(), nullptr);
            for (size_t i = 0; i < arr.size(); ++i) {
                if (arr[i].isObject()) {
                    const auto modPtr = labModule_fromJson(arr[i].object(), depsRec, context.binder);
                    if (modPtr) {
                        context.modStorage.insert_module_ptr_dangerous(modPtr);
                        context.id_to_module[i] = modPtr;
                    }
                }
            }
        }
    }

    for (auto& rec : depsRec) {
        for (auto& val : *rec.depsArr) {
            if (val.isObject()) {
                const auto& depObj = val.object();
                const auto modIdVal = depObj.find("module_id");
                const auto depFound = findModuleById(context, modIdVal);
                if (depFound) {
                    DependencySymbolInfo* info = nullptr;
                    const auto infoObj = depObj.find("info");
                    if (infoObj && infoObj->isObject()) {
                        info = dependencySymbolInfo_fromJson(infoObj->object(), context);
                    }
                    rec.module->add_dependency(depFound, info);
                }
            }
        }
    }

    if (auto exeArr = obj.find("executables")) {
        if (exeArr->isArray()) {
            for (auto& exe : exeArr->array()) {
                if (exe.isObject()) {
                    const auto exePtr = labJob_fromJson(exe.object(), context);
                    if (exePtr) {
                        context.jobs.emplace_back(exePtr);
                    }
                }
            }
        }
    }
}

bool labBuildContext_fromJson(BuildContextInformation& context, std::string_view jsonContent) {
    auto thing = lsp::json::parse(jsonContent);
    if (thing.isObject()) {
        labBuildContext_fromJson(context, thing.object());
        return true;
    }
    return false;
}