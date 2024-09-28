// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "lexer/Lexi.h"
#include "utils/lspfwd.h"
#include <future>
#include "integration/cbi/model/LexResult.h"
#include "integration/cbi/model/ASTResult.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/ASTImportUnit.h"
#include "integration/cbi/model/ImportUnitCache.h"

class RemoteEndPoint;

class GlobalInterpretScope;

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

//    /**
//     * map between absolute path of a file and their mutexes
//     * when you require a file is lexed, a mutex is held for each path
//     * so multiple different paths can be lexed at a single time but multiple same paths cannot.
//     */
//    std::unordered_map<std::string, std::mutex> parse_file_mutexes;

    /**
     * a mutex for the unordered_map access, this is because every call with a path must be processed sequentially
     * otherwise parallel calls might render lex_file_mutexes useless
     */
    std::mutex lex_file_mutexes_map_mutex;

//    /**
//     * a mutex for the unordered_map access, this is because every call with a path must be processed sequentially
//     * otherwise parallel calls might render lex_file_mutexes useless
//     */
//    std::mutex parse_file_mutexes_map_mutex;

    /**
     * import unit cache, contains different import units
     */
    ImportUnitCache cache;

    /**
     * the argv is the path to the lsp executable
     */
    std::string lsp_exe_path;

    /**
     * the compiler binder to use in this entire process
     */
    CompilerBinder binder;

    /**
     * notify_diagnostics is triggered when the client asks for semantic tokens
     * we use this mutex to launch instances of publish diagnostics without causing race conditions
     */
    std::mutex publish_diagnostics_mutex;
    /**
     * the task used by publish diagnostics
     */
    std::future<void> publish_diagnostics_task;
    /**
     * this flag can be used to cancel the current running diagnostics task
     */
    std::atomic<bool> publish_diagnostics_cancel_flag{false};

    /**
     * this flag can be used to cancel requests doing hard work
     */
    std::atomic<bool> cancel_request{false};

public:

    /**
     * the remote end point, we can use this to send diagnostics to client
     * send notifications and stuff, it's our connection to IDE
     */
    RemoteEndPoint* remote;

    /**
     * path to resources folder, if empty, will be calculated relative to current executable
     */
    std::string overridden_resources_path;

    /**
     * currently is64Bit is determined at compile time
     */
    bool is64Bit = sizeof(void*) == 8;

    /**
     * constructor
     */
    explicit WorkspaceManager(std::string lsp_exe_path);

    /**
     * will determine compiler executable path, relative to the current lsp exe path
     */
    std::string compiler_exe_path();

    /**
     * get the path to resources folder
     */
    std::string resources_path();

    /**
     * get a system header translated to the output path output_path
     * pair contains output from command line and executable return status code
     */
    std::pair<std::string, int> get_c_translated(const std::string& header_abs_path, const std::string& output_path);

    /**
     * get the folding range for the given absolute file path
     */
    td_foldingRange::response get_folding_range(const lsDocumentUri& uri);

    /**
     * get completion response for the given absolute file path
     * @param line the line number where caret position is
     * @param character the character number where caret position is
     */
    td_completion::response get_completion(const lsDocumentUri& uri, unsigned int line, unsigned int character);

    /**
     * get semantic tokens for the given document uri
     */
    std::vector<SemanticToken> get_semantic_tokens(const std::string& abs_path);

    /**
     * get semantic tokens full response for the given document uri
     */
    td_semanticTokens_full::response get_semantic_tokens_full(const lsDocumentUri& uri);

    /**
     * get definition at position in the given document
     */
    td_definition::response get_definition(const lsDocumentUri& uri, const lsPosition& position);

    /**
     * get symbols in the document
     */
    td_symbol::response get_symbols(const lsDocumentUri& uri);

    /**
     * get a hover response in the given document at position
     */
    td_hover::response get_hover(const lsDocumentUri& uri, const lsPosition& position);

    /**
     * get text document links response
     */
    td_links::response get_links(const lsDocumentUri& uri);

    /**
     * it has to copy all the diagnostics to a request before sending
     * the request is done async
     */
    void notify_diagnostics_async(
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags
    );

    /**
     * this will publish given diagnostics
     */
    void notify_diagnostics(
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags
    );

    /**
     * this will publish complete diagnostics for the given file, non asynchronously
     */
    void publish_diagnostics_complete(
        const std::string& path,
        bool notify_async,
        std::atomic<bool>& cancel_flag
    );

    /**
     * after the first invocation, every other invocation is queued
     */
    template<typename TaskLambda>
    void queued_single_invocation(
        std::mutex& task_mutex,
        std::future<void>& task,
        std::atomic<bool>& cancel_flag,
        const TaskLambda& lambda
    );

    /**
     * will publish diagnostics
     */
    void publish_diagnostics_complete_async(
        const std::string& path,
        std::launch launch_policy,
        bool notify_async,
        bool do_synchronous
    );

    /**
     * check if lex import unit has errors
     */
    static bool has_errors(const LexImportUnit& unit);

    /**
     * check if this ast import unit has errors
     */
    static bool has_errors(const ASTImportUnit& unit);

    /**
     * get the import unit for the given absolute path
     */
    LexImportUnit get_import_unit(const std::string& abs_path, std::atomic<bool>& cancel_flag);

    /**
     * get the ast import unit for the given lex import unit
     */
    ASTImportUnit get_ast_import_unit(const LexImportUnit& unit, std::atomic<bool>& cancel_flag);

    /**
     * it will symbol resolver the ast import unit
     * it will return diagnostics of symbol resolution for the last file in import unit
     */
    std::vector<Diag> sym_res_import_unit(ASTImportUnit& unit, std::atomic<bool>& cancel_flag);

    /**
     * get a locked mutex for this path only
     */
    std::mutex& lex_lock_path_mutex(const std::string& path);

    /**
     * get a locked mutex for this path only
     */
    std::mutex& parse_lock_path_mutex(const std::string& path);

    /**
     * get the lexed file, only if it exists in cache
     */
    std::shared_ptr<LexResult> get_cached(const std::string& path);

    /**
     * get the ast result, only if it exists in cache
     */
    std::shared_ptr<ASTResult> get_cached_ast(const std::string& path);

    /**
     * get tokens for the given file
     *
     * Lexes the file contents, The contents can be either
     * 1 - In memory contents of the file (user has made changes to file which aren't saved on disk)
     * 2 - Direct file contents (the file in the IDE is as its present on disk)
     *
     * NOTE: this doesn't support C headers | C files
     * please use the function which access FlatIGFile, which also requires the path in the import statement
     * because that path contains whether it's a system header
     */
    std::shared_ptr<LexResult> get_lexed(const std::string& path);

    /**
     * same as get_lexed, however this doesn't lock a mutex or protect against multiple calls
     * from different threads
     */
    std::shared_ptr<LexResult> get_lexed_no_lock(const std::string& path);

    /**
     * gets the lex result, then converts to ASTResult
     *
     * this will also cache the ASTResult and provide it back
     */
    std::shared_ptr<ASTResult> get_ast(
        const std::string& path,
        GlobalInterpretScope& comptime_scope
    );

    /**
     * just like the get_lexed above, but this supports C headers and files because
     * it checks the path in the import statement, if it's a system header or a normal C file
     * it stores the C headers in the lib/system/stdio.h for example
     * relative to the lsp executable path
     */
    std::shared_ptr<LexResult> get_lexed(const FlatIGFile& flat_file);

    /**
     * get the tokens only for the given file path
     */
    std::vector<CSTToken*>& get_lexed_tokens(const std::string& path) {
        return get_lexed(path)->unit.tokens;
    }

    /**
     * Returns the overridden source code for file at path
     */
    std::optional<std::string> get_overridden_source(const std::string& path);

    /**
     * will give canonical path, when provided an absolute path, or empty string
     * will also report in cerr if couldn't determine canonical path
     */
    std::string canonical(const std::string& path);

    /**
     * Its called with the changes that have been performed to the contents of a file in the IDE \n
     * Then reads the file, performs the changes to source code (in memory) \n
     * Then calls onChangedContents above to store the changed source coe as overridden contents \n
     */
    void onChangedContents(const lsDocumentUri &uri, const std::vector<lsTextDocumentContentChangeEvent>& changes);

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

/**
 * get's header path in lib/system directory when given header path like stdio.h
 */
std::string rel_to_lib_system(const std::string& header_path, const std::string& lsp_exe_path);

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