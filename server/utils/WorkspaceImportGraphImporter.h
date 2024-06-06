// Copyright (c) Qinetik 2024.

#include "preprocess/ImportGraphMaker.h"

class WorkspaceImportGraphImporter : public ImportGraphImporter {
public:

    WorkspaceManager *manager;

    WorkspaceImportGraphImporter(
            ImportPathHandler *handler,
            Lexer *lexer,
            ImportGraphVisitor *converter,
            WorkspaceManager *manager
    );

    std::vector<IGFile> process(const std::string &path, IGFile *parent) override;

};