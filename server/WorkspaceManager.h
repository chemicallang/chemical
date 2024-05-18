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
#include "lexer/Lexi.h"
#include "utils/lspfwd.h"
#include "integration/ide/model/LexResult.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/ImportUnitCache.h"

class RemoteEndPoint;

/**
 * Workspace manager is the operations manager for all IDE related operations
 * 1 - IDE needs semantic highlighting for a single file
 * 2 - IDE needs completion items
 * 3 - User wants to rename a symbol
 *
 * Workspace manager provides functions that take into account
 * caching of files, caching of CST & AST, dispatching operations
 *
 * It also takes into account, user workspace configuration, The main job of the
 * workspace manager is to oversee the process of integration and interaction with IDEs
 *
 * It must support at a single time, LSP operations for multiple projects in a single workspace
 *
 * It will outlive a session between our LSP and IDE, user opens IDE, LSP starts
 * along with workspace manager, the instance will die when the IDE closes.
 */
class WorkspaceManager {
private:

    /**
     * overridden sources contain user edited files
     * when user edits, they aren't saved directly to disk, so we store edited state here
     */
    std::unordered_map<std::string, std::string> overriddenSources;

    /**
     * This mutex is for the change of files
     * Changes to a file are delivered in sequential order, meaning two requests will modify the source at the same time
     * or the second request might be processed completely before the first has finished
     * causing the source code to be modified by the second request and leading to unexpected changes
     */
    std::mutex incremental_change_mutex;

    /**
     * map between absolute path of a file and their mutexes
     * when you require a file is lexed, a mutex is held for each path
     * so multiple different paths can be lexed at a single time but multiple same paths cannot.
     */
    std::unordered_map<std::string, std::mutex> lex_file_mutexes;

    /**
     * a mutex for the unordered_map access, this is because every call with a path must be processed sequentially
     * otherwise parallel calls might render lex_file_mutexes useless
     */
    std::mutex lex_file_mutexes_map_mutex;

    /**
     * import unit cache, contains different import units
     */
    ImportUnitCache cache;

public:

    /**
     * the remote end point, we can use this to send diagnostics to client
     * send notifications and stuff, it's our connection to IDE
     */
    RemoteEndPoint* remote;

    /**
     * get the folding range for the given absolute file path
     */
    td_foldingRange::response get_folding_range(const std::string& abs_path);

    /**
     * get completion response for the given absolute file path
     * @param line the line number where caret position is
     * @param character the character number where caret position is
     */
    td_completion::response get_completion(const std::string& abs_path, unsigned int line, unsigned int character);

    /**
     * get semantic tokens for the given document uri
     */
    std::vector<SemanticToken> get_semantic_tokens(const lsDocumentUri& uri);

    /**
     * get semantic tokens full response for the given document uri
     */
    td_semanticTokens_full::response get_semantic_tokens_full(const lsDocumentUri& uri);

    /**
     * get definition at position in the given document
     */
    td_definition::response get_definition(const lsDocumentUri& uri, const lsPosition& position);

    /**
     * this will publish given diagnostics
     */
    void publish_diagnostics(const std::string& path, bool async, const std::vector<std::vector<Diag>*>& diags);

    /**
     * get the import unit for the given absolute path
     */
    ImportUnit get_import_unit(const std::string& abs_path, bool publish_diagnostics = false);

    /**
     * get tokens for the given file
     *
     * Lexes the file contents, The contents can be either
     * 1 - In memory contents of the file (user has made changes to file which aren't saved on disk)
     * 2 - Direct file contents (the file in the IDE is as its present on disk)
     */
    std::shared_ptr<LexResult> get_lexed(const std::string& path);

    /**
     * get the tokens only for the given file path
     */
    std::vector<std::unique_ptr<CSTToken>>& get_lexed_tokens(const std::string& path) {
        return get_lexed(path)->tokens;
    }

    /**
     * Returns the overridden source code for file at path
     */
    std::optional<std::string> get_overridden_source(const std::string& path);

    /**
     * Its called with the changes that have been performed to the contents of a file in the IDE \n
     * Then reads the file, performs the changes to source code (in memory) \n
     * Then calls onChangedContents above to store the changed source coe as overridden contents \n
     */
    void onChangedContents(const std::string& path, const std::vector<lsTextDocumentContentChangeEvent>& changes);

    /**
     * when a file is closed by the user in the IDE \n
     * this function removes any cache associated with that file \n
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