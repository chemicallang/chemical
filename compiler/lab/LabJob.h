// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "std/chem_string.h"
#include "LabJobType.h"
#include "preprocess/StringViewHashEqual.h"
#include "compiler/cbi/model/CBIData.h"
#include "compiler/cbi/model/CBIFunctionIndex.h"
#include "std/unordered_map.h"
#include "TargetData.h"
#include "compiler/OutputMode.h"
#include "import_model/RemoteImport.h"
#include "import_model/ModuleDependency.h"

struct LabModule;

enum class LabJobStatus : int {
    Pending = 0,
    Launched = 1,
    Success = 2,
    Failure = 3
};

enum class ConflictResolutionStrategy : int {
    Default = 0,
    PreferNewerVersion = 1,
    PreferOlderVersion = 2,
    RaiseError = 3,
    OverridePrevious = 4,
    KeepPrevious = 5
};

struct LabJob {

    /**
     * the type of job, the type is user specific
     * which type of job user want's to perform
     */
    LabJobType type;

    /**
     * name of the job / executable / lib
     */
    chem::string name;

    /**
     * absolute path to the job's output (exe or lib)
     */
    chem::string abs_path;

    /**
     * absolute path to build dir for this job
     */
    chem::string build_dir;

    /**
     * the status of the job
     */
    LabJobStatus status = LabJobStatus::Pending;

    /**
     * if a job is optional, upon failure, we don't quit doing other jobs
     */
    bool optional_job = false;

    /**
     * the target triple for every job can be different
     * and therefore is stored here, by default the target triple supplied via
     * the command line is used here, or default system target (if not supplied)
     */
    chem::string target_triple;

    /**
     * output mode for this job
     */
    OutputMode mode;

    /**
     * the target data
     */
    TargetData target_data = create_target_data();

    /**
     * these are linkable object or bitcode files required by the job
     */
    std::vector<chem::string> objects;

    /**
     * the libraries to be linked (-l cmd param)
     */
    std::vector<chem::string> link_libs;

    /**
     * paths user asked to specify for searching libraries
     */
    std::vector<chem::string> lib_search_paths;

    /**
     * files to be shipped (copied to output directory)
     */
    std::vector<chem::string> ship_files;

    /**
     * dependencies are the pointers to modules that this job depends on
     * these modules will be compiled first
     */
    std::vector<ModuleDependency> dependencies;

    /**
     * remote imports to be downloaded and added as dependencies
     */
    std::vector<std::unique_ptr<RemoteImport>> remote_imports;

    /**
     * index for remote imports to avoid linear searches
     * key: origin/scope/name, value: index in remote_imports
     */
    std::unordered_map<std::string, size_t> remote_import_index;

    /**
     * definitions are user defined build variables, that user can use at compile
     * time to trigger different code paths and generate different code
     */
    std::unordered_map<std::string, bool> definitions;

    /**
     * built files are the build files (.mod or .lab) that have been built
     * this is used to avoid building the same file multiple times
     * .lab and .mod files perform checks using their absolute path before building
     */
    util::unordered_string_map<LabModule*> built_files;

    /**
     * a mutex for this job, it protects access and write on dependencies, built_files...etc
     */
    std::mutex mutex;

    /**
     * the conflict resolution strategy for remote imports
     */
    ConflictResolutionStrategy conflict_strategy = ConflictResolutionStrategy::PreferNewerVersion;

    /**
     * constructor
     */
    inline LabJob(
            LabJobType type,
            chem::string name,
            OutputMode mode
    ) : type(type), name(std::move(name)), mode(mode) {

    }

    /**
     * constructor
     */
    inline LabJob(
            LabJobType type,
            chem::string name,
            chem::string abs_path,
            chem::string build_dir,
            OutputMode mode
    ) : type(type), name(std::move(name)), abs_path(std::move(abs_path)), build_dir(std::move(build_dir)), mode(mode) {

    }

    /**
     * add a module dependency on this job
     */
    void add_dependency(LabModule* dependency, DependencySymbolInfo* info = nullptr) {
        dependencies.emplace_back(dependency, info);
    }

    /**
     * reserve total dependencies before adding
     */
    void reserve_dependencies(std::size_t size) {
        dependencies.reserve(size);
    }

};

struct LabJobCBI : public LabJob {
public:

    /**
     * indexes are the functions that user asked us to index
     * these functions would be found when this job is done, these would
     * be called when required
     */
    std::vector<CBIFunctionIndex> indexes;

    /**
     * constructor
     */
    inline LabJobCBI(
            chem::string name,
            OutputMode mode
    ) : LabJob(LabJobType::CBI, std::move(name), mode) {

    }

    /**
     * constructor
     */
    inline LabJobCBI(
            chem::string name,
            chem::string abs_path,
            chem::string build_dir,
            OutputMode mode
    ) : LabJob(LabJobType::CBI, std::move(name), std::move(abs_path), std::move(build_dir), mode) {

    }

};