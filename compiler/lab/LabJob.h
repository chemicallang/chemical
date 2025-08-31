// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <unordered_map>
#include "std/chem_string.h"
#include "LabJobType.h"
#include "preprocess/StringViewHashEqual.h"
#include "compiler/cbi/model/CBIData.h"
#include "compiler/cbi/model/CBIFunctionIndex.h"
#include "TargetData.h"
#include "compiler/OutputMode.h"

struct LabModule;

enum class LabJobStatus {
    Pending = 0,
    Launched = 1,
    Success = 2,
    Failure = 3
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
    std::vector<chem::string> linkables;

    /**
     * dependencies are the pointers to modules that this job depends on
     * these modules will be compiled first
     */
    std::vector<LabModule*> dependencies;

    /**
     * path aliases are used to basically alias a path using '@'
     * when user will import using an '@' we will replace it with the actual path
     */
    std::unordered_map<std::string, std::string, StringHash, StringEqual> path_aliases;

    /**
     * definitions are user defined build variables, that user can use at compile
     * time to trigger different code paths and generate different code
     */
    std::unordered_map<std::string, bool> definitions;

    /**
     * constructor
     */
    inline LabJob(
            LabJobType type,
            chem::string name
    ) : type(type), name(std::move(name)) {

    }

    /**
     * constructor
     */
    inline LabJob(
            LabJobType type,
            chem::string name,
            chem::string abs_path,
            chem::string build_dir
    ) : type(type), name(std::move(name)), abs_path(std::move(abs_path)), build_dir(std::move(build_dir)) {

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
            chem::string name
    ) : LabJob(LabJobType::CBI, std::move(name)) {

    }

    /**
     * constructor
     */
    inline LabJobCBI(
            chem::string name,
            chem::string abs_path,
            chem::string build_dir
    ) : LabJob(LabJobType::CBI, std::move(name), std::move(abs_path), std::move(build_dir)) {

    }

};