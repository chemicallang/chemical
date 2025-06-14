// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "utils/lspfwd.h"
#include "utils/LRUCache.h"
#include <future>
#include "server/model/LexResult.h"
#include "server/model/ASTResult.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "ast/base/TypeBuilder.h"
#include "preprocess/ImportPathHandler.h"
#include "std/chem_string_view.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/ModuleStorage.h"
#include "lsp/types.h"
#include "compiler/lab/LabBuildContext.h"
#include "ctpl.h"
#include "server/model/ModuleData.h"
#include "core/source/LocationManager.h"
#include "compiler/processor/ModuleFileData.h"

class GlobalInterpretScope;

struct LabJob;

namespace lsp {
    class MessageHandler;
}

struct CachedModuleUnit {
    /**
     * the module file units belong to
     */
    LabModule* module;
    /**
     * constructor
     */
    CachedModuleUnit(LabModule* module) : module(module) {

    }
};

/**
 * Workspace manager is the operations manager for all IDE related operations for a
 * single client
 *
 * 1 - IDE needs semantic highlighting for a single file
 * 2 - IDE needs completion items
 * 3 - User wants to rename a symbol
 *
 * Workspace manager provides functions that take into account
 * caching of files, caching of AST, dispatching operations
 *
 * It also takes into account, user workspace configuration, The main job of the
 * workspace manager is to oversee the process of integration and interaction with IDEs
 * for a single client
 *
 * It must support at a single time, LSP operations for multiple projects in a single workspace
 *
 * It will outlive a session between our LSP and a client, user opens IDE, LSP starts
 * along with workspace manager, the instance will die when the client disconnects
 */
class WorkspaceManager {
public:

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
     * global container is created once for multiple threads, this mutex is used to not
     * trigger creation of multiple global containers from different threads
     */
    std::mutex global_container_mutex;

    /**
     * module data mutex is used to synchronize creation of corresponding module data for each module
     * module data contains additional data and state required for processing it
     */
    std::mutex module_data_mutex;

    /**
     * why ? because semantic tokens request requires access to tokens inside which
     * linked pointers to ast nodes/values/types exist
     */
    LRUCache<std::string, std::shared_ptr<LexResult>> tokenCache;

    /**
     * we build indexes of file to module pointers, this allows us to know the module
     * given file in an instant
     */
    std::unordered_map<chem::string_view, ModuleData*> filesIndex;

    /**
     * each module's data is stored here
     */
    std::unordered_map<LabModule*, std::unique_ptr<ModuleData>> moduleData;

    /**
     * these modules have changed, they contain a unordered set of files
     * which tells you exactly which files have changed
     */
    std::unordered_set<ModuleData*> dirtyModules;

    /**
     * if user is editing .mod file, we keep its module file data
     * in this lru cache, tokens are stored in usual token cache
     */
    LRUCache<std::string, std::shared_ptr<ModuleFileDataUnit>> modFileData;

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
     * this context is used to store information about module graph
     */
    BasicBuildContext context;

    /**
     * path to resources folder, if empty, will be calculated relative to current executable
     */
    std::string overridden_resources_path;

    /**
     * the thread pool is used to launch jobs
     */
    ctpl::thread_pool pool;

    /**
     * we have a pointer to the main job
     * by default, first job in the build.lab is considered the main job
     * path aliases of the given job are used
     */
    LabJob* main_job = nullptr;

    /**
     * this container is created once, it holds all the global functions
     * and classes, like intrinsics namespace and std namespace is by default
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
    void initialize(const lsp::InitializeParams &req);

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
     * indexes the module's files (the modules present in module storage)
     */
    void index_module_files();

    /**
     * when build.lab has been built, we can build modules
     * or do whateer we want with the data we have received
     */
    void post_build_lab();

    /**
     * compile a .lab file or a .mod file
     * this method is called in a separate process to handle compilation
     * this method reports to parent process
     * the lab build context is a new pointer, you are taking ownership of it
     */
    static LabBuildContext* compile_lab(const std::string& exe_path, const std::string& path, ModuleStorage& storage);

    /**
     * builds the context from a build.lab or chemical.mod file present in project path
     */
    int build_context_from_build_lab();

    /**
     * this creates the global container (once) or binds (if already created), method
     * is safe to use from multiple threads
     */
    void bind_or_create_container(GlobalInterpretScope& comptime_scope, SymbolResolver& resolver);

    /**
     * get module for the given file
     */
    ModuleData* getModuleData(const chem::string_view& filePath);

    /**
     * get module data for a module
     */
    ModuleData* getModuleData(LabModule* module);

    /**
     * makes the module dirty (must be symbol resolved, before next request comes in)
     */
    void make_module_dirty(ModuleData* modData, CachedASTUnit* dirtyFile) {
        modData->make_single_file_dirty(dirtyFile);
        dirtyModules.insert(modData);
    }

    /**
     * removed the module from dirty modules (has been symbol resolved)
     */
    void unmake_module_dirty(ModuleData* modData) {
        modData->set_completely_symbol_resolved();
        dirtyModules.erase(modData);
    }

    /**
     * before request, a file can be prepared, in this case
     * if this file is dirty, or a file it depends upon is dirty (not symbol resolved)
     * is symbol resolved so that everything is ready to serve the request (tokens, unit)
     */
    bool should_process_file(const std::string& path, ModuleData* modData);

    /**
     * this allows processing the file to place unit
     */
    void process_file(const std::string& path, bool contents_changed, bool depends_on_dirty);

    /**
     * process a dot mod file
     */
    void process_dot_mod_file(const std::string& path);

    /**
     * processes any file including .mod and build.lab files
     */
    void process_any_file(const std::string& path, bool contents_changed, bool depends_on_dirty);

    /**
     * process file on open (contents haven't changed, if tokens & ast exist in cache, can be skipped)
     */
    void process_any_file_on_open(const std::string& path);

    /**
     * process the file only if it need by
     */
    inline void process_file_on_request(const std::string& path, ModuleData* modData) {
        if(modData && should_process_file(path, modData)) {
            process_file(path, false, true);
        }
    }

    /**
     * process the file only if it need by
     */
    inline void process_file_on_request(const std::string& path) {
        const auto modData = getModuleData(chem::string_view(path));
        if(modData && should_process_file(path, modData)) {
            process_file(path, false, true);
        }
    }

    /**
     * get the folding range for the given absolute file path
     */
    std::vector<lsp::FoldingRange> get_folding_range(const std::string_view& path);

    /**
     * get completion response for the given absolute file path
     * @param line the line number where caret position is
     * @param character the character number where caret position is
     */
    lsp::CompletionList get_completion(const std::string_view& path, const Position& position);

    /**
     * get semantic tokens for the given lex result
     */
    std::vector<uint32_t> get_semantic_tokens(LexResult& ptr);

    /**
     * get semantic tokens full response for the given document uri
     */
    std::vector<uint32_t> get_semantic_tokens_full(const std::string_view& path);

    /**
     * get definition at position in the given document
     */
    std::vector<lsp::DefinitionLink> get_definition(const std::string_view& path, const Position& position);

    /**
     * get symbols in the document
     */
    std::vector<lsp::DocumentSymbol> get_symbols(const std::string_view& path);

    /**
     * get a hover response in the given document at position
     */
    std::string get_hover(const std::string_view& path, const Position& position);

//    /**
//     * get text document links response
//     */
//    td_links::response get_links(const lsDocumentUri& uri);

    /**
     * get text document hints response
     */
    std::vector<lsp::InlayHint> get_hints(const std::string_view& path, const Range& range);

    /**
     * get signature help response
     */
    lsp::SignatureHelp get_signature_help(const std::string_view& path, const Position& position);

    /**
     * if a new file is added, we try to find its module and index it
     */
    void index_new_file(const std::string_view& path);

    /**
     * this file will be removed from the index since user deleted it
     * the module of this file will not contain this file after this function
     */
    void de_index_deleted_file(const std::string_view& path);

    /**
     * registers the watched files capability to receive watched files events
     */
    void register_watched_files_capability();

    /**
     * this will send given diagnostics by sending a notification to client
     */
    void notify_diagnostics(
            const std::string& path,
            std::vector<lsp::Diagnostic> diags
    );

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
     * will publish diagnostics
     */
    void publish_diagnostics(const std::string& path, std::vector<lsp::Diagnostic> diagnostics);

    /**
     * get ast, in which declarations are sure to be valid
     */
    std::shared_ptr<ASTResult> get_decl_ast(const std::string& abs_path);

    /**
     * same as the method below
     */
    bool get_lexed(LexResult* result, const std::string& path, bool keep_comments = false);

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
    std::shared_ptr<LexResult> get_lexed(const std::string& path, bool keep_comments = false);

    /**
     * gets the ast no locking or cache hit
     */
    std::shared_ptr<ASTResult> get_ast_no_lock(
            Token* start_token,
            const std::string& path
    );

    /**
     * Returns the overridden source code for file at path
     */
    std::optional<std::string> get_overridden_source(const std::string& path);

    /**
     * should be called when a file is opened
     */
    void OnOpenedFile(const std::string_view& uri);

    /**
     * Its called with the changes that have been performed to the contents of a file in the IDE \n
     * Then reads the file, performs the changes to source code (in memory) \n
     * Then calls onChangedContents above to store the changed source coe as overridden contents \n
     */
    void onChangedContents(const std::string_view &uri, const std::vector<lsp::TextDocumentContentChangeEvent>& changes);

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