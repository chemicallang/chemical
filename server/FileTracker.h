// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "lexer/model/tokens/LexToken.h"
#include "lexer/LexConfig.h"
#include "lexer/Lexi.h"
#include "LibLsp/lsp/textDocument/did_change.h"

class FileTracker {

private:

    std::unordered_map<std::string, std::string> overriddenSources;

    /**
     * This mutex is for the change of files
     * Changes to a file are delivered in sequential order, meaning two requests will modify the source at the same time
     * or the second request might be processed completely before the first has finished
     * causing the source code to be modified by the second request and leading to unexpected changes
     */
    std::mutex incremental_change_mutex;

public:

    /**
     * Lexes the file contents, The contents can be either \n
     * 1 - In memory contents of the file (user has made changes to file which aren't saved on disk) \n
     * 2 - Direct file contents (the file in the IDE is as its present on disk) \n
     * @param path
     * @return
     */
    std::vector<std::unique_ptr<LexToken>> getLexedFile(const std::string& path, const LexConfig& config);

    /**
     * Returns the overridden source code for file at path
     * @param path
     * @return
     */
    std::string getOverriddenSource(const std::string& path);

    /**
     * stores the overridden (changed) contents of the file \n
     * This happens when user has changes in the IDE that aren't present on the file in disk \n
     * so next time when lexing is performed, it returns lexing using overridden contents \n
     * @param contents
     */
    void onChangedContents(const std::string& path, const std::string& contents);

    /**
     * Its called with the changes that have been performed to the contents of a file in the IDE \n
     * Then reads the file, performs the changes to source code (in memory) \n
     * Then calls onChangedContents above to store the changed source coe as overridden contents \n
     * @param path
     * @param changes
     */
    void onChangedContents(const std::string& path, const std::vector<lsTextDocumentContentChangeEvent>& changes);

    /**
     * when a file is closed by the user in the IDE \n
     * this function removes any cache associated with that file \n
     * @param path
     */
    void onClosedFile(const std::string& path);

    /**
     * when the editor is closed, or all files are closed, or just to erase all caches
     * to provide lexing from the files on disk rather than their IDE state, this function can be called
     */
    void clearAllStoredContents();

};

extern void replace(
        std::string &source,
        unsigned int lineStart,
        unsigned int charStart,
        unsigned int lineEnd,
        unsigned int charEnd,
        const std::string &replacement
);

extern void replaceSafe(std::string &source, unsigned int lineStart, unsigned int charStart, unsigned int lineEnd,
                        unsigned int charEnd, const std::string &replacement);