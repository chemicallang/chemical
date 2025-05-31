// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "utils/lspfwd.h"
#include <future>
#include "compiler/cbi/model/LexResult.h"
#include "compiler/cbi/model/ASTResult.h"
#include "compiler/cbi/model/LexImportUnit.h"
#include "compiler/cbi/model/ASTImportUnitRef.h"
#include "compiler/cbi/model/ImportUnitCache.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "ast/base/TypeBuilder.h"
#include "preprocess/ImportPathHandler.h"
#include "std/chem_string_view.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/ModuleStorage.h"
#include "lsp/types.h"

class LabBuildContext;

class RemoteEndPoint;

class GlobalInterpretScope;

class LSPLabImpl;

struct LabJob;

namespace lsp {
    class MessageHandler;
}

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
     * the argv is the path to the lsp executable
     */
    std::string lsp_exe_path;

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
    std::unordered_map<std::string, std::mutex> parse_file_mutexes;

    /**
     * we build indexes of file to module pointers, this allows us to know the module
     * given file in an instant
     */
    std::unordered_map<chem::string_view, LabModule*> filesIndex;

    /**
     * a single location manager is used throughout
     * This presents some challenges, for example large file locations
     * are stored on this location manager, location manager must dispose
     * locations when cached files change
     */
    LocationManager loc_man;

    /**
     * the global allocator allocates memory for entire session
     */
    ASTAllocator global_allocator;

    /**
     * module storage is used to store and index modules
     */
    ModuleStorage modStorage;

    /**
     * a type builder holds cache of types to allow reusing them
     */
    TypeBuilder typeBuilder;

    /**
     * the path handler
     */
    ImportPathHandler pathHandler;

    /**
     * a mutex for the unordered_map access, this is because every call with a path must be processed sequentially
     * otherwise parallel calls might render lex_file_mutexes useless
     */
    std::mutex lex_file_mutexes_map_mutex;

    /**
     * the mutex used when getting ast import units, only a single ast import
     * unit can be requested
     */
    std::mutex ast_import_unit_mutex;

    /**
     * import unit cache, contains different import units
     */
    ImportUnitCache cache;

    /**
     * when initialize request is sent from client, project path is saved
     */
    std::string project_path;

    /**
     * the compiler binder to use in this entire process
     */
    CompilerBinder binder;

    /**
     * message handler
     */
    lsp::MessageHandler& handler;

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
     * lab build compiler
     */
    LSPLabImpl* lab = nullptr;

    /**
     * we have a pointer to the main job
     * by default, first job in the build.lab is considered the main job
     * path aliases of the given job are used
     */
    LabJob* main_job = nullptr;

    /**
     * this container is created once, it holds all the global functions
     * and classes, like compiler namespace and std namespace is by default
     * provided by the compiler
     */
    GlobalContainer* global_container = nullptr;

    /**
     * currently is64Bit is determined at compile time
     */
    bool is64Bit = sizeof(void*) == 8;

    /**
     * constructor
     */
    explicit WorkspaceManager(std::string lsp_exe_path, lsp::MessageHandler& handler);

    /**
     * get module for the given file
     */
    LabModule* get_mod_of(const chem::string_view& filePath);

    /**
     * gets the module and makes sure declarations for all its direct files exist
     */
    std::future<void> trigger_sym_res(LabModule* module);

    /**
     * this get the current target triple
     */
    std::string get_target_triple();

    /**
     * will determine compiler executable path, relative to the current lsp exe path
     */
    std::string compiler_exe_path();

    /**
     * get the path to resources folder
     */
    std::string resources_path();

    /**
     * initialize
     */
//    void initialize(const td_initialize::request &req);

    /**
     * determines build.lab path relative to project path
     */
    std::string get_mod_file_path();

    /**
     * determines build.lab path relative to project path
     */
    std::string get_build_lab_path();

    /**
     * this switches the main job to the given job
     * it's path aliases will be used
     */
    void switch_main_job(LabJob* job);

    /**
     * when build.lab has been built, we can build modules
     * or do whateer we want with the data we have received
     */
    void post_build_lab(LabBuildCompiler* compiler);

    /**
     * this compiles build.lab, also notifies the user
     */
    bool compile_build_lab();

    /**
     * get the folding range for the given absolute file path
     */
    std::vector<lsp::FoldingRange> get_folding_range(const std::string& path);
//
//    /**
//     * get completion response for the given absolute file path
//     * @param line the line number where caret position is
//     * @param character the character number where caret position is
//     */
//    td_completion::response get_completion(const lsDocumentUri& uri, unsigned int line, unsigned int character);
//
//    /**
//     * get semantic tokens for the given lex result
//     */
    std::vector<uint32_t> get_semantic_tokens(LexResult& ptr);

    /**
     * get semantic tokens full response for the given document uri
     */
    std::vector<uint32_t> get_semantic_tokens_full(const std::string& path);
//
//    /**
//     * get definition at position in the given document
//     */
//    td_definition::response get_definition(const lsDocumentUri& uri, const lsPosition& position);
//
    /**
     * get symbols in the document
     */
    std::vector<lsp::DocumentSymbol> get_symbols(const std::string& path);
//
//    /**
//     * get a hover response in the given document at position
//     */
//    td_hover::response get_hover(const lsDocumentUri& uri, const lsPosition& position);
//
//    /**
//     * get text document links response
//     */
//    td_links::response get_links(const lsDocumentUri& uri);
//
//    /**
//     * get text document hints response
//     */
//    td_inlayHint::response get_hints(const lsDocumentUri& uri);
//
//    /**
//     * get signature help response
//     */
//    td_signatureHelp::response get_signature_help(const lsDocumentUri& uri, const lsPosition& position);

    /**
     * it has to copy all the diagnostics to a request before sending
     * the request is done async
     */
    void notify_diagnostics_async(
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags
    );

    /**
     * this will send given diagnostics by sending a notification to client
     */
    void notify_diagnostics_sync(
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags
    );

    /**
     * a helper function
     * allows to choose whether notification is prepared and sent asynchronously
     */
    inline void notify_diagnostics(
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags,
        bool async
    ) {
        async ? notify_diagnostics_async(path, diags) : notify_diagnostics_sync(path, diags);
    }

    /**
     * publish diagnostics synchronously
     */
    void publish_diagnostics_for_sync(ASTImportUnitRef& ref, bool notify_async);

    /**
     * publish diagnostics asynchronously for the given ast import unit ref
     */
    std::future<void> publish_diagnostics_for_async(ASTImportUnitRef& ref);

    /**
     * this allows publishing diagnostics for the given ast import unit ref
     * @param do_async allows to collect diagnostics and publish them
     */
    inline void publish_diagnostics_for(ASTImportUnitRef& ref, bool async) {
        if(async) {
            publish_diagnostics_for_async(ref);
        } else {
            publish_diagnostics_for_sync(ref, true);
        }
    }

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
    void publish_diagnostics(std::shared_ptr<LexResult> file);

    /**
     * check if lex import unit has errors
     */
    static bool has_errors(const std::vector<std::shared_ptr<LexResult>>& lexFiles);

    /**
     * check if given ast files has errors
     */
    static bool has_errors(const std::vector<std::shared_ptr<ASTResult>>& files);

    /**
     * get the import unit for the given absolute path
     * this function just returns ast unit for the given path and nothing else
     */
    [[deprecated]]
    LexImportUnit get_import_unit(const std::string& abs_path, std::atomic<bool>& cancel_flag);

    /**
     * get the ast import unit for the given lex import unit into the files vector
     */
    void get_ast_import_unit(
        std::vector<std::shared_ptr<ASTResult>>& files,
        const LexImportUnit& unit,
        GlobalInterpretScope& comptime_scope,
        std::atomic<bool>& cancel_flag
    );

    /**
     * it will symbol resolver the ast import unit
     * it will return diagnostics of symbol resolution for the last file in import unit
     */
    std::vector<Diag> sym_res_import_unit(
        std::vector<std::shared_ptr<ASTResult>>& files,
        GlobalInterpretScope& comptime_scope,
        std::atomic<bool>& cancel_flag
    );

    /**
     * get ast, in which declarations are sure to be valid
     */
    std::shared_ptr<ASTResult> get_decl_ast(const std::string& abs_path);

    /**
     * get ast import unit for the following name
     */
    ASTImportUnitRef get_ast_import_unit(const std::string& abs_path, std::atomic<bool>& cancel_flag);

    /**
     * get a locked mutex for this path only
     */
    std::mutex& parse_lock_path_mutex(const std::string& path);

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
     * gets the ast no locking or cache hit
     */
    std::shared_ptr<ASTResult> get_ast_no_lock(
            Token* start_token,
            const std::string& path
    );

    /**
     * gets the lex result, then converts to ASTResult
     *
     * this will also cache the ASTResult and provide it back
     */
    std::shared_ptr<ASTResult> get_ast(
            LexResult* result,
            GlobalInterpretScope& comptime_scope
    );

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
    void onChangedContents(const std::string &uri, const std::vector<lsp::TextDocumentContentChangeEvent>& changes);

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

    /**
     * destructor
     */
    ~WorkspaceManager();

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