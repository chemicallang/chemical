// Copyright (c) Chemical Language Foundation 2026.

#include "TransformerContextCBI.h"
#include "compiler/lab/transformer/TransformerContext.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "compiler/ASTProcessor.h"
#include "compiler/lab/LabJob.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabBuildContext.h"
#include "CBIUtils.h"

LabJob* TransformerContextgetTargetJob(TransformerContext* self) {
    return self->job;
}

bool TransformerContextparseTarget(TransformerContext* self, bool keep_comments) {
    for(const auto target : self->flattened_mods) {
        const auto res = self->processor->import_chemical_files_direct_with_tokens(
                self->compiler->pool,
                target->direct_files,
                self->file_tokens,
                keep_comments
        );
        if(res == false) return false;
    }
    return true;
}

bool TransformerContextanalyzeTarget(TransformerContext* self) {

    const auto processor = self->processor;

    for(const auto target : self->flattened_mods) {

        // symbol resolution
        if (processor->sym_res_module(target) != 0) return false;

        // type verification
        if(!processor->type_verify_module_parallel(self->compiler->pool, target)) return false;

    }

    return true;

}

std::vector<LabModule*>* TransformerContextgetFlattenedModules(TransformerContext* self) {
    return &self->flattened_mods;
}

void TransformerContextgetFileTokens(FileTokensSpanCBI* out, TransformerContext* self, unsigned int fileId) {
    auto it = self->file_tokens.find(fileId);
    if (it != self->file_tokens.end()) {
        *out = { it->second.data(), it->second.size() };
    } else {
        *out = { nullptr, 0 };
    }
}

void TransformerContextdecodeLocation(LocationDataCBI* out, TransformerContext* self, uint64_t encoded) {
    auto data = self->compiler->loc_man.getLocation(encoded);
    out->fileId = data.fileId;
    out->lineStart = data.lineStart;
    out->charStart = data.charStart;
    out->lineEnd = data.lineEnd;
    out->charEnd = data.charEnd;
}

void ModulegetFiles(ASTFileMetaDataSpanCBI* out, LabModule* self) {
    *out = ASTFileMetaDataSpanCBI { self->direct_files.data(), self->direct_files.size() };
}

std::size_t ModulegetFileCount(LabModule* self) {
    return self->direct_files.size();
}

ASTFileMetaData* ModulegetFile(LabModule* self, unsigned int index) {
    return &self->direct_files[index];
}

unsigned int ModulegetDependencyCount(LabModule* self) {
    return (unsigned int)self->dependencies.size();
}

LabModule* ModulegetDependency(LabModule* self, unsigned int index) {
    return self->dependencies[index].module;
}

unsigned int FileMetaDatagetFileId(ASTFileMetaData* self) {
    return self->file_id;
}

void FileMetaDatagetAbsPath(chem::string_view* view, ASTFileMetaData* self) {
    *view = chem::string_view(self->abs_path.data(), self->abs_path.size());
}

FileScope* FileMetaDatagetFileScope(ASTFileMetaData* self) {
    if (!self->result) return nullptr;
    return &self->result->unit.scope;
}

Scope* FileScopegetBody(FileScope* self) {
    return &self->body;
}