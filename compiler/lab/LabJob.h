// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <unordered_map>
#include "std/chem_string.h"
#include "LabJobType.h"
#include "parser/model/CBIData.h"

struct LabModule;

enum class LabJobStatus {
    Pending = 0,
    Launched = 1,
    Success = 2,
    Failure = 3
};

struct LabJob {
    // the type of job, the type is user specific
    // which type of job user want's to perform
    LabJobType type;
    // name of the job / executable / lib
    chem::string name;
    // absolute path to the job's output (exe or lib)
    chem::string abs_path;
    // absolute path to build dir for this job
    chem::string build_dir;
    // the status of the job
    LabJobStatus status = LabJobStatus::Pending;
    // these are linkable object or bitcode files required by the job
    std::vector<chem::string> linkables;
    // dependencies are the pointers to modules that this job depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;
    /**
     * path aliases are used to basically alias a path using '@'
     * when user will import using an '@' we will replace it with the actual path
     */
    std::unordered_map<std::string, std::string> path_aliases;
    /**
     * definitions are user defined build variables, that user can use at compile
     * time to trigger different code paths and generate different code
     */
    std::unordered_map<std::string, bool> definitions;
};

struct LabJobCBI : public LabJob {
    /**
     * the entry module is provided when cbi is created
     * it is looked for all public functions like lex, parse and others
     */
    LabModule* entry_module = nullptr;
};