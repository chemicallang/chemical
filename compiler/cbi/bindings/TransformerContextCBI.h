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

struct ASTFileMetaDataSpanCBI {
    ASTFileMetaData* ptr;
    std::size_t size;
};

struct FileTokensSpanCBI {
    Token* ptr;
    std::size_t size;
};

extern "C" {
    LabJob* TransformerContextgetTargetJob(TransformerContext* self);
    bool TransformerContextparseTarget(TransformerContext* self, bool keep_comments);
    bool TransformerContextanalyzeTarget(TransformerContext* self);
    std::vector<LabModule*>* TransformerContextgetFlattenedModules(TransformerContext* self);
    void TransformerContextgetFileTokens(FileTokensSpanCBI* out, TransformerContext* self, unsigned int fileId);
    void TransformerContextdecodeLocation(LocationDataCBI* out, TransformerContext* self, uint64_t encoded);

    // FileMetaData getters
    void ModulegetFiles(ASTFileMetaDataSpanCBI* out, LabModule* self);
    unsigned int FileMetaDatagetFileId(ASTFileMetaData* self);
    void FileMetaDatagetAbsPath(chem::string_view* view, ASTFileMetaData* self);
    FileScope* FileMetaDatagetFileScope(ASTFileMetaData* self);
    Scope* FileScopegetBody(FileScope* self);

}