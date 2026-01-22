// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include "compiler/lab/LabModuleType.h"
#include "compiler/lab/LabJob.h"

struct LabModule;
struct LabJob;
struct TargetData;

extern "C" {

    // Module getters/setters
    int ModulegetType(LabModule* self);
    void ModulegetScopeName(chem::string_view* view, LabModule* self);
    void ModulegetName(chem::string_view* view, LabModule* self);
    
    void ModulegetBitcodePath(chem::string_view* view, LabModule* self);
    void ModulesetBitcodePath(LabModule* self, chem::string_view* path);
    
    void ModulegetObjectPath(chem::string_view* view, LabModule* self);
    void ModulesetObjectPath(LabModule* self, chem::string_view* path);
    
    void ModulegetLlvmIrPath(chem::string_view* view, LabModule* self);
    void ModulesetLlvmIrPath(LabModule* self, chem::string_view* path);
    
    void ModulegetAsmPath(chem::string_view* view, LabModule* self);
    void ModulesetAsmPath(LabModule* self, chem::string_view* path);

    // LabJob getters
    int LabJobgetType(LabJob* self);
    void LabJobgetName(chem::string_view* view, LabJob* self);
    void LabJobgetAbsPath(chem::string_view* view, LabJob* self);
    void LabJobgetBuildDir(chem::string_view* view, LabJob* self);
    int LabJobgetStatus(LabJob* self);
    void LabJobgetTargetTriple(chem::string_view* view, LabJob* self);
    int LabJobgetMode(LabJob* self);
    TargetData* LabJobgetTarget(LabJob* self);

}
