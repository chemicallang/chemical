// Copyright (c) Chemical Language Foundation 2025.

#include "preprocess/ImportGraphMaker.h"

class WorkspaceImportGraphImporter : public ImportGraphImporter {
public:

    WorkspaceManager *manager;

    WorkspaceImportGraphImporter(
            ImportPathHandler *handler,
            Parser *lexer,
            ImportGraphVisitor *converter,
            WorkspaceManager *manager
    );

    std::vector<IGFile> process(const std::string &path, const Range& range, IGFile *parent) final;

};