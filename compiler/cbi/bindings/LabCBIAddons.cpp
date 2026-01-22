// Copyright (c) Chemical Language Foundation 2025.

#include "LabCBIAddons.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabJob.h"

extern "C" {

    int ModulegetType(LabModule* self) {
        return static_cast<int>(self->type);
    }

    void ModulegetScopeName(chem::string_view* view, LabModule* self) {
        *view = self->scope_name.to_chem_view();
    }

    void ModulegetName(chem::string_view* view, LabModule* self) {
        *view = self->name.to_chem_view();
    }

    void ModulegetBitcodePath(chem::string_view* view, LabModule* self) {
        *view = self->bitcode_path.to_chem_view();
    }

    void ModulesetBitcodePath(LabModule* self, chem::string_view* path) {
        self->bitcode_path.clear();
        self->bitcode_path.append(*path);
    }

    void ModulegetObjectPath(chem::string_view* view, LabModule* self) {
        *view = self->object_path.to_chem_view();
    }

    void ModulesetObjectPath(LabModule* self, chem::string_view* path) {
        self->object_path.clear();
        self->object_path.append(*path);
    }

    void ModulegetLlvmIrPath(chem::string_view* view, LabModule* self) {
        *view = self->llvm_ir_path.to_chem_view();
    }

    void ModulesetLlvmIrPath(LabModule* self, chem::string_view* path) {
        self->llvm_ir_path.clear();
        self->llvm_ir_path.append(*path);
    }

    void ModulegetAsmPath(chem::string_view* view, LabModule* self) {
        *view = self->asm_path.to_chem_view();
    }

    void ModulesetAsmPath(LabModule* self, chem::string_view* path) {
        self->asm_path.clear();
        self->asm_path.append(*path);
    }

    int LabJobgetType(LabJob* self) {
        return static_cast<int>(self->type);
    }

    void LabJobgetName(chem::string_view* view, LabJob* self) {
        *view = self->name.to_chem_view();
    }

    void LabJobgetAbsPath(chem::string_view* view, LabJob* self) {
        *view = self->abs_path.to_chem_view();
    }

    void LabJobgetBuildDir(chem::string_view* view, LabJob* self) {
        *view = self->build_dir.to_chem_view();
    }

    int LabJobgetStatus(LabJob* self) {
        return static_cast<int>(self->status);
    }

    void LabJobgetTargetTriple(chem::string_view* view, LabJob* self) {
        *view = self->target_triple.to_chem_view();
    }

    int LabJobgetMode(LabJob* self) {
        return static_cast<int>(self->mode);
    }

    TargetData* LabJobgetTarget(LabJob* self) {
        return &self->target_data;
    }

}
