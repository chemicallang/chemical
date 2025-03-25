// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include "LabModuleType.h"
#include "integration/cbi/model/FlatIGFile.h"
#include "std/chem_string.h"
#include "ast/structures/ModuleScope.h"

struct LabModule {

    /**
     * type of the module
     */
    LabModuleType type;

    /**
     * the scope name of the module
     */
    chem::string scope_name;

    /**
     * name of the module
     */
    chem::string name;

    /**
     * the translated c file (if any) will be written to this path
     */
    chem::string out_c_path;

    /**
     * bitcode file path for this module
     */
    chem::string bitcode_path;

    /**
     * object file path for this module
     */
    chem::string object_path;

    /**
     * if not empty, module's llvm ir is written to at this path
     */
    chem::string llvm_ir_path;

    /**
     * if not empty, module's assembly is written to at this path
     */
    chem::string asm_path;

    /**
     * module scope
     */
    ModuleScope module_scope;

    /**
     * these headers are imported before any other files are processed
     */
    std::vector<chem::string> headers;

    /**
     * this path point to a c (.c, .h) file, a chemical file, a directory or a build.lab
     * depends on the type of module
     */
    std::vector<chem::string> paths;

    /**
     * dependencies are the pointers to modules that this module depends on
     * these modules will be compiled first
     */
    std::vector<LabModule*> dependencies;

    /**
     * dependents are the pointers to modules that depend on this module
     * these modules will be compiled after this module has been compiled
     */
    std::vector<LabModule*> dependents;

    /**
     * constructor
     */
    LabModule(
            LabModuleType mod_type,
            chem::string scope_name,
            chem::string module_name
    ) : type(mod_type), scope_name(std::move(scope_name)), name(std::move(module_name)),
        module_scope(this->scope_name.to_chem_view(), this->name.to_chem_view())
    {

    }

    /**
     * this allows updating the module and scope names
     */
    void update_mod_name(chem::string new_scope_name, chem::string new_module_name) {
        scope_name = std::move(new_scope_name);
        name = std::move(new_module_name);
        module_scope.scope_name = this->scope_name.to_chem_view();
        module_scope.module_name = this->name.to_chem_view();
    }

    /**
     * get the dependencies of this module
     */
    const std::vector<LabModule*>& get_dependencies() {
        return dependencies;
    }

    /**
     * get the dependents of this module
     */
    const std::vector<LabModule*>& get_dependents() {
        return dependents;
    }

    /**
     * add a module dependency into this module
     */
    inline void add_dependency(LabModule* dependency) {
        dependencies.emplace_back(dependency);
        dependency->dependents.emplace_back(this);
    }

    /**
     * will add the given dependencies into this module
     */
    void add_dependencies(const std::vector<LabModule*>& deps) {
        for(const auto dep : deps) {
            add_dependency(dep);
        }
    }

};