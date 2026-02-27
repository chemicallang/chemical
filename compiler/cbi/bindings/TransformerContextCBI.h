#pragma once

#include <cstdint>
#include "compiler/processor/ASTFileResult.h"
#include "ast/structures/FileScope.h"

class TransformerContext;
struct LabJob;
struct LabModule;
struct Token;

struct LocationDataCBI {
    uint32_t fileId;
    uint32_t lineStart;
    uint32_t charStart;
    uint32_t lineEnd;
    uint32_t charEnd;
};

extern "C" {
    LabJob* TransformerContextgetTargetJob(TransformerContext* self);
    bool TransformerContextparseTarget(TransformerContext* self, bool keep_comments);
    bool TransformerContextanalyzeTarget(TransformerContext* self);
    std::vector<LabModule*>* TransformerContextgetFlattenedModules(TransformerContext* self);
    std::vector<Token>* TransformerContextgetFileTokens(TransformerContext* self, unsigned int fileId);
    void TransformerContextdecodeLocation(LocationDataCBI* out, TransformerContext* self, uint64_t encoded);

    // FileMetaData getters
    std::vector<ASTFileMetaData>* ModulegetFiles(LabModule* self);
    unsigned int FileMetaDatagetFileId(ASTFileMetaData* self);
    void FileMetaDatagetAbsPath(chem::string_view* view, ASTFileMetaData* self);
    FileScope* FileMetaDatagetFileScope(ASTFileMetaData* self);
    Scope* FileScopegetBody(FileScope* self);

}