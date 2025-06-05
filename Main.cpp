
// Copyright (c) Chemical Language Foundation 2025.

#include <lsp/messages.h> // Generated message definitions
#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/io/stream.h>
#include <lsp/io/socket.h>
#include <iostream>
#include "server/WorkspaceManager.h"
#include "utils/FileUtils.h"
#include "utils/CmdUtils.h"
#include "utils/CmdUtils2.h"
#include "utils/Version.h"
#include "core/main/CompilerMain.h"

#include <thread>
#include <atomic>
#include <functional>
#include <utility>
#include "utils/JsonUtils.h"
#include "server/build/ChildProcessBuild.h"

#include <sstream>
#include <iostream>
#include <memory>
#include <thread>

// 2) Per-client session: runs the LSP loop until error or disconnect
void run_session(
        std::atomic_bool& g_shutdown,
        lsp::io::SocketListener& listener,
        lsp::io::Socket socket,
        const char* executable_path
) {
  try {

    bool local_shutdown = false;
//    SocketStream stream();
    lsp::Connection connection(socket);
    lsp::MessageHandler handler(connection);
    WorkspaceManager manager(executable_path, handler);

    handler.add<lsp::requests::Initialize>([&manager](lsp::InitializeParams&& params){

        // initializing the manager
        manager.initialize(params);

        std::ostringstream versionStr;
        versionStr << 'v' << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH;

        std::variant<lsp::TextDocumentSyncOptions, lsp::TextDocumentSyncKindEnum> textDocSync;
        textDocSync.emplace<lsp::TextDocumentSyncOptions>(
            true, lsp::TextDocumentSyncKind::Incremental, false, false, std::nullopt
        );

        std::variant<lsp::SemanticTokensOptions, lsp::SemanticTokensRegistrationOptions> tokensProvider;
         tokensProvider.emplace<lsp::SemanticTokensOptions>(
             lsp::SemanticTokensOptions{
                 .workDoneProgress = false,
                 .legend = {
                     {"namespace","type","class","enum","interface","struct","typeparameter","parameter","variable","property","enummember","event","function","method","macro","keyword","modifier","comment","string","number","regexp","operator","decorator", },
                    { "declaration","definition","readonly","static","deprecated","abstract","async","modification","documentation","defaultlibrary", }
                 },
                 .range = false,
                 .full = true
             }
         );

        std::variant<bool, lsp::FoldingRangeOptions, lsp::FoldingRangeRegistrationOptions> foldingOptions;
        foldingOptions.emplace<lsp::FoldingRangeOptions>(lsp::FoldingRangeOptions{
                .workDoneProgress = std::nullopt
        });

        std::variant<bool, lsp::DocumentSymbolOptions> symbolOptions;
        symbolOptions.emplace<lsp::DocumentSymbolOptions>(lsp::DocumentSymbolOptions{
                .workDoneProgress = std::nullopt,
                .label = std::nullopt
        });

        return lsp::requests::Initialize::Result{
                .capabilities = {
                        .textDocumentSync = textDocSync,
                        .documentSymbolProvider = symbolOptions,
                        .foldingRangeProvider = foldingOptions,
                        .semanticTokensProvider = tokensProvider,
                },
                .serverInfo = lsp::InitializeResultServerInfo{
                        .name    = "Chemical Language Server",
                        .version = versionStr.str()
                }
        };

    });

    handler.add<lsp::requests::TextDocument_SemanticTokens_Full>([&manager](lsp::requests::TextDocument_SemanticTokens_Full::Params&& params){
        auto tokens = manager.get_semantic_tokens_full(params.textDocument.uri.path());
        return lsp::requests::TextDocument_SemanticTokens_Full::Result(lsp::SemanticTokens{
            .data = tokens
        });
    });

    handler.add<lsp::requests::TextDocument_FoldingRange>([&manager](lsp::requests::TextDocument_FoldingRange::Params&& params){
      return lsp::Nullable(manager.get_folding_range(params.textDocument.uri.path()));
    });

    handler.add<lsp::requests::TextDocument_DocumentSymbol>([&manager](lsp::requests::TextDocument_DocumentSymbol::Params&& params){
        return lsp::NullableVariant<std::vector<lsp::SymbolInformation>, std::vector<lsp::DocumentSymbol>>(manager.get_symbols(params.textDocument.uri.path()));
    });

    handler.add<lsp::notifications::TextDocument_DidOpen>([&manager](lsp::notifications::TextDocument_DidOpen::Params&& params){
        // TODO: The file should be indexed with implementation
    });

    handler.add<lsp::notifications::TextDocument_DidChange>([&manager](lsp::notifications::TextDocument_DidChange::Params&& params){
        manager.onChangedContents(params.textDocument.uri.path(), params.contentChanges);
    });

    handler.add<lsp::requests::Shutdown>([&listener, &local_shutdown, &g_shutdown]() -> std::nullptr_t {
        std::cout << "[LSP] Shutdown requested." << std::endl;
        // 1) mark both local and global shutdown
        local_shutdown = true;
        g_shutdown.store(true, std::memory_order_relaxed);
        return nullptr;
    });

    // ====================================

    // Main loop: will block inside processIncomingMessages()
    while (!local_shutdown) {
      handler.processIncomingMessages();
    }

    // Now the Shutdown response has gone out, we can close.
    std::cout << "[LSP] Shutting down socket." << std::endl;
    socket.close();

    // If this was *the* session that asked us to shut down,
    // we should also close the acceptor (to break out of main’s accept loop):
    if (g_shutdown.load(std::memory_order_relaxed)) {
        listener.shutdown();
    }
    std::cout << "[LSP] Session thread exiting." << std::endl;

  } catch (const lsp::RequestError& e) {
    // If you throw RequestError in a handler, lsp-framework
    // will automatically send an error response; we just log it here.
    std::cerr << "[LSP][RequestError] " << e.code() << ": " << e.what() << "\n";
  } catch (const std::exception& e) {
    std::cerr << "[LSP][Fatal] Unexpected exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "[LSP][Fatal] Unknown error\n";
  }
  // thread exits, socket closed on destruction
}

//bool ShouldIgnoreFileForIndexing(const std::string &path) {
//    return StartsWith(path, "git:");
//}

//class Server {
//public:
//
//    RemoteEndPoint &_sp;
//    WorkspaceManager manager;
//    std::optional<Rsp_Error> need_initialize_error;
//    std::unique_ptr<ParentProcessWatcher> parent_process_watcher;
//    std::shared_ptr<ClientPreferences> clientPreferences;
//
//    struct ExitMsgMonitor {
//        std::atomic_bool is_running_ = true;
//
//        bool isCancelled() {
//            return !is_running_.load(std::memory_order_relaxed);
//        }
//
//        void Cancel() {
//            is_running_.store(false, std::memory_order_relaxed);
//        }
//    };
//
//    ExitMsgMonitor exit_monitor;
//
//    void on_exit() {
//        exit_monitor.Cancel();
//        server.stop();
//        esc_event.notify(std::make_unique<bool>(true));
//    }
//
//    std::map<std::string, Registration> registeredCapabilities;
//
//    void collectRegisterCapability(const std::string &method) {
//        if (registeredCapabilities.find(method) == registeredCapabilities.end()) {
//            auto reg = Registration::Create(method);
//            registeredCapabilities[method] = reg;
//        }
//    }
//
//    Server(
//            const std::string &user_agent,
//            const std::string &_port,
//            bool _enable_watch_parent_process,
//            std::string lsp_exe_path
//    ) : _sp(server.point), server(_address, _port, protocol_json_handler, endpoint, _log), manager(std::move(lsp_exe_path)) {
//
//        manager.remote = &server.point;
//        need_initialize_error = Rsp_Error();
//        need_initialize_error->error.code = lsErrorCodes::ServerNotInitialized;
//        need_initialize_error->error.message = "Server is not initialized";
//
//        _sp.registerHandler([=](const chemical_meta::request &req){
//            std::ostringstream versionStr;
//            versionStr << 'v' << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH;
//            chemical_meta::response rsp;
//            rsp.result.version = versionStr.str();
//            return std::move(rsp);
//        });
//
//        _sp.registerHandler([=](const td_initialize::request &req) {
//
//            _log.log(lsp::Log::Level::INFO, "td_initialize");
//
//            manager.initialize(req);
//
//            need_initialize_error.reset();
//            clientPreferences = std::make_shared<ClientPreferences>(req.params.capabilities);
//            td_initialize::response rsp;
//            lsServerCapabilities capabilities;
//            auto SETTINGS_KEY = "settings";
//
//            if (req.params.initializationOptions) {
//                do {
//                    map<std::string, lsp::Any> initializationOptions;
//                    try {
//                        lsp::Any init_object = req.params.initializationOptions.value();
//
//                        init_object.Get(initializationOptions);
//                    }
//                    catch (...) {
//                        break;
//                    }
//                    map<std::string, lsp::Any> settings;
//                    try {
//
//                        initializationOptions[SETTINGS_KEY].Get(settings);
//                    }
//                    catch (...) {
//                        break;
//                    }
//
//                } while (false);
//            }
//
//
//            std::pair<optional<lsTextDocumentSyncKind>, optional<lsTextDocumentSyncOptions>> textDocumentSync;
//            lsTextDocumentSyncOptions options;
//            options.openClose = true;
//            options.change = lsTextDocumentSyncKind::Incremental;
//            options.willSave = false;
//            options.willSaveWaitUntil = false;
//            textDocumentSync.second = options;
//            capabilities.textDocumentSync = textDocumentSync;
//
//			    if(!clientPreferences->isHoverDynamicRegistered())
//			    {
//					capabilities.hoverProvider = true;
//			    }
//				if (!clientPreferences->isCompletionDynamicRegistered())
//				{
//					lsCompletionOptions completion;
//					completion.resolveProvider = true;
//					capabilities.completionProvider = completion;
//				}
//				std::pair< optional<bool>, optional<WorkDoneProgressOptions> > option;
//				option.first = true;
//
//				if (!clientPreferences->isDefinitionDynamicRegistered())
//				{
//					capabilities.definitionProvider = { true, std::nullopt };
//				}
//				if (!clientPreferences->isFoldgingRangeDynamicRegistered())
//				{
//					capabilities.foldingRangeProvider = std::pair< optional<bool>, optional<FoldingRangeOptions> >();
//					capabilities.foldingRangeProvider->first = true;
//				}
//                if (!clientPreferences->isInlayHintDynamicRegistered())
//                {
//                    capabilities.inlayHintProvider = std::pair< optional<bool>, optional<InlayHintOptions> >();
//                    capabilities.inlayHintProvider->first = true;
//                }
//                if (!clientPreferences->isSignatureHelpDynamicRegistrationSupported()) {
//                    capabilities.signatureHelpProvider = lsSignatureHelpOptions {};
//                }
////				if (!clientPreferences->isReferencesDynamicRegistered())
////				{
////					capabilities.referencesProvider = option;
////				}
//				if (!clientPreferences->isDocumentSymbolDynamicRegistered())
//				{
//					capabilities.documentSymbolProvider = option;
//				}
////				if (!clientPreferences->isFormattingDynamicRegistrationSupported())
////				{
////					capabilities.documentFormattingProvider = option;
////				}
////				if (!clientPreferences->isRenameDynamicRegistrationSupported())
////				{
////					std::pair< boost::optional<bool>, boost::optional<RenameOptions> > rename_opt;
////					rename_opt.first = true;
////					capabilities.renameProvider = rename_opt;
////				}
//
////            {
//
//            SemanticTokensWithRegistrationOptions semantic_tokens_opt;
//            auto semanticTokenTypes = [] {
//                std::vector<std::string> _type;
//                for (unsigned i = 0; i <= static_cast<unsigned>(SemanticTokenType::lastKind);
//                     ++i)
//                    _type.push_back(to_string(static_cast<SemanticTokenType>(i)));
//                return _type;
//            };
//            auto semanticTokenModifiers = [] {
//                std::vector<std::string> _type;
//                for (unsigned i = 0; i <= static_cast<unsigned>(SemanticTokenModifier::LastModifier);
//                     ++i)
//                    _type.push_back(to_string(static_cast<SemanticTokenModifier>(i)));
//                return _type;
//            };
//
//            semantic_tokens_opt.legend.tokenTypes = semanticTokenTypes();
//            semantic_tokens_opt.legend.tokenModifiers = semanticTokenModifiers();
//
//            std::pair<optional<bool>, optional<lsp::Any> > rang;
//            rang.first = false;
//            semantic_tokens_opt.range = rang;
//
//            std::pair<optional<bool>,
//                    optional<SemanticTokensServerFull> > full;
//            full.first = true;
//
//            semantic_tokens_opt.full = full;
//            capabilities.semanticTokensProvider = std::move(semantic_tokens_opt);
////            }
//
//            rsp.result.capabilities.swap(capabilities);
//            WorkspaceServerCapabilities workspace_server_capabilities;
//            //capabilities.workspace
//            if (req.params.processId.has_value() && _enable_watch_parent_process) {
//                parent_process_watcher = std::make_unique<ParentProcessWatcher>(_log, req.params.processId.value(),
//                                                                                [&]() {
//                                                                                    on_exit();
//                                                                                });
//            }
//
////            _log.log(lsp::Log::Level::INFO, rsp.ToJson());
//
//            return std::move(rsp);
//        });
//        _sp.registerHandler([&](Notify_InitializedNotification::notify &notify) {
//
//            _log.log(lsp::Log::Level::INFO, "Notify_InitializedNotification");
//
//            if (!clientPreferences)return;
//
//            _log.log(lsp::Log::Level::INFO, "Notify_InitializedNotification ClientPrefs Set");
//
//            if (clientPreferences->isCompletionDynamicRegistered()) {
//                collectRegisterCapability(td_completion::request::kMethodInfo);
//            }
//            if (clientPreferences->isWorkspaceSymbolDynamicRegistered()) {
//                collectRegisterCapability(wp_symbol::request::kMethodInfo);
//            }
//            if (clientPreferences->isDocumentSymbolDynamicRegistered()) {
//                collectRegisterCapability(td_symbol::request::kMethodInfo);
//            }
//            if(clientPreferences->isDocumentLinkDynamicRegistered()) {
//                  collectRegisterCapability(td_links::request::kMethodInfo);
//            }
//            if(clientPreferences->isInlayHintDynamicRegistered()) {
//                collectRegisterCapability(td_inlayHint::request::kMethodInfo);
//            }
//            /*if (clientPreferences->isCodeActionDynamicRegistered())
//            {
//                collectRegisterCapability(td_codeAction::request::kMethodInfo);
//            }*/
//            if (clientPreferences->isDefinitionDynamicRegistered()) {
//                collectRegisterCapability(td_definition::request::kMethodInfo);
//            }
//            if (clientPreferences->isHoverDynamicRegistered()) {
//                collectRegisterCapability(td_hover::request::kMethodInfo);
//            }
//            if (clientPreferences->isReferencesDynamicRegistered()) {
//                collectRegisterCapability(td_references::request::kMethodInfo);
//            }
//            if (clientPreferences->isFoldgingRangeDynamicRegistered()) {
//                collectRegisterCapability(td_foldingRange::request::kMethodInfo);
//            }
//            if (clientPreferences->isWorkspaceFoldersSupported()) {
//                collectRegisterCapability(Notify_WorkspaceDidChangeWorkspaceFolders::notify::kMethodInfo);
//            }
//            if (clientPreferences->isWorkspaceDidChangeConfigurationSupported()) {
//                collectRegisterCapability(Notify_WorkspaceDidChangeConfiguration::notify::kMethodInfo);
//            }
//            if (clientPreferences->isSemanticHighlightingSupported()) {
//                collectRegisterCapability(td_semanticTokens_full::request::kMethodInfo);
//            }
//            if (clientPreferences->isSignatureHelpSupported()) {
//                collectRegisterCapability(td_signatureHelp::request::kMethodInfo);
//            }
//
//            Req_ClientRegisterCapability::request request;
//            for (auto &it: registeredCapabilities) {
//                request.params.registrations.push_back(it.second);
//            }
//            _sp.send(request);
//
//        });
//        _sp.registerHandler(
//                [&](const td_definition::request &req, const CancelMonitor &monitor)
//                        -> lsp::ResponseOrError<td_definition::response> {
//                    _log.log(lsp::Log::Level::INFO, "td_definition");
//                    if (need_initialize_error) {
//                        return need_initialize_error.value();
//                    }
//                return manager.get_definition(req.params.textDocument.uri, req.params.position);;
//        });
//        _sp.registerHandler([&](const td_symbol::request &req) -> lsp::ResponseOrError<td_symbol::response> {
//            _log.log(lsp::Log::Level::INFO, "td_symbol");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_symbols(req.params.textDocument.uri);
//        });
//        _sp.registerHandler(
//                [&](const td_hover::request &req, const CancelMonitor &monitor)
//                        -> lsp::ResponseOrError<td_hover::response> {
//                    _log.log(lsp::Log::Level::INFO, "td_hover");
//                    if (need_initialize_error) {
//                        return need_initialize_error.value();
//                    }
//                    return manager.get_hover(req.params.textDocument.uri, req.params.position);
//                });
//        _sp.registerHandler([&](const td_completion::request &req, const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_completion::response> {
//            _log.log(lsp::Log::Level::INFO, "td_completion");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_completion(req.params.textDocument.uri, req.params.position.line, req.params.position.character);
//        });
//        _sp.registerHandler([&](const completionItem_resolve::request &req) {
//            _log.log(lsp::Log::Level::INFO, "completionItem_resolve");
//            completionItem_resolve::response rsp;
//            rsp.result = req.params;
//            return std::move(rsp);
//        });
//
//        _sp.registerHandler([&](const td_foldingRange::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_foldingRange::response> {
//            _log.log(lsp::Log::Level::INFO, "td_foldingRange");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_folding_range(req.params.textDocument.uri);
//        });
//        _sp.registerHandler([&](const td_formatting::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_formatting::response> {
//            _log.log(lsp::Log::Level::INFO, "td_formatting");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            td_formatting::response rsp;
////				auto unit = GetUnit(req.params.textDocument);
////				if (unit){
////					DocumentFormatHandler(unit, rsp.result, req.params.options);
////				}
//            return std::move(rsp);
//        });
//        _sp.registerHandler([&](const td_links::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_links::response> {
//            _log.log(lsp::Log::Level::INFO, "td_links");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_links(req.params.textDocument.uri);
//        });
//        _sp.registerHandler([&](const td_linkResolve::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_linkResolve::response> {
//            _log.log(lsp::Log::Level::INFO, "td_linkResolve");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            // No need to support this request, since we send resolved links to the IDE
//            td_linkResolve::response rsp;
//            return std::move(rsp);
//        });
//        _sp.registerHandler([&](const td_inlayHint::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_inlayHint::response> {
//            _log.log(lsp::Log::Level::INFO, "td_inlayHint");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_hints(req.params.textDocument.uri);
//        });
//        _sp.registerHandler([&](const td_signatureHelp::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_signatureHelp::response> {
//            _log.log(lsp::Log::Level::INFO, "td_signatureHelp");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_signature_help(req.params.textDocument.uri, req.params.position);
//        });
//        _sp.registerHandler([&](const td_inlayHintResolve::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_inlayHintResolve::response> {
//            _log.log(lsp::Log::Level::INFO, "td_inlayHintResolve");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            // No need to support this request, since we send resolved hints to the IDE
//            td_inlayHintResolve::response rsp;
//            return std::move(rsp);
//        });
////        _sp.registerHandler([&](const td_documenColor::request &req,
////                                const CancelMonitor &monitor)
////                                    -> lsp::ResponseOrError<td_documentColor::response> {
////            _log.log(lsp::Log::Level::INFO, "td_documentColor");
////            if (need_initialize_error) {
////                return need_initialize_error.value();
////            }
////            td_documentColor::response rsp;
////
//////				auto unit = GetUnit(req.params.textDocument);
//////				if (unit){
//////					DocumentColorHandler(unit, rsp.result);
//////				}
////            return std::move(rsp);
////        });
//        auto x = 0;
//        _sp.registerHandler([&](const td_semanticTokens_full::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_semanticTokens_full::response> {
//            _log.log(lsp::Log::Level::INFO, "td_semanticTokens_full");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            return manager.get_semantic_tokens_full(req.params.textDocument.uri);
//        });
////        _sp.registerHandler([&](const td_references::request &req,
////                                const CancelMonitor &monitor)
////                                    -> lsp::ResponseOrError<td_references::response> {
////            _log.log(lsp::Log::Level::INFO, "td_references");
////            if (need_initialize_error) {
////                return need_initialize_error.value();
////            }
////            td_references::response rsp;
//////				auto unit = GetUnit(req.params.textDocument);
//////				if (unit){
//////					ReferencesHandler(unit, req.params.pos, rsp.result,&_requestMonitor);
//////				}
////            return std::move(rsp);
////        });
////        _sp.registerHandler([&](const td_rename::request &req,
////                                const CancelMonitor &monitor)
////                                    -> lsp::ResponseOrError<td_rename::response> {
////            _log.log(lsp::Log::Level::INFO, "td_rename");
////            if (need_initialize_error) {
////                return need_initialize_error.value();
////            }
////            td_rename::response rsp;
//////				auto unit = GetUnit(req.params.textDocument);
//////				if (unit) {
////            std::vector<lsWorkspaceEdit::Either> edits;
//////					RenameHandler(unit, req.params, edits, &_requestMonitor);
////            rsp.result.documentChanges = std::move(edits);
////
//////				}
////            return std::move(rsp);
////        });
//        _sp.registerHandler([&](Notify_TextDocumentDidOpen::notify &notify) {
//
//            _log.log(lsp::Log::Level::INFO, "TextDocumentDidOpen Received");
//
//            if (need_initialize_error) {
//                return;
//            }
//
//            _log.log(lsp::Log::Level::INFO, "TextDocumentDidOpen Received 2");
//
//            auto &params = notify.params;
//            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
//            if (ShouldIgnoreFileForIndexing(path))
//                return;
////				work_space_mgr.OnOpen(params.textDocument);
//        });
//
//        _sp.registerHandler([&](Notify_TextDocumentDidChange::notify &notify) {
//
//            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received 1");
//
//            if (need_initialize_error) {
//                return;
//            }
//
//            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received\n");
//
//            // to json the request
////            _log.log(lsp::Log::Level::INFO, notify.ToJson());
////            _log.log(lsp::Log::Level::INFO, "\nAbove was the json");
//            const auto &params = notify.params;
//            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
//
//            if(params.contentChanges.empty()) {
//                _log.log(lsp::Log::Level::INFO, "No Changes in the code");
//            }
//
//            manager.onChangedContents(params.textDocument.uri, params.contentChanges);
//
//            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received 3");
//
//        });
//
//        _sp.registerHandler([&](Notify_TextDocumentDidClose::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_TextDocumentDidClose");
//
//            if (need_initialize_error) {
//                return;
//            }
//            //Timer time;
//            const auto &params = notify.params;
//            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
//            if (ShouldIgnoreFileForIndexing(path))
//                return;
//
//            manager.onClosedFile(path.path);
//
//        });
//
//        _sp.registerHandler([&](Notify_TextDocumentDidSave::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_TextDocumentDidSave");
//            if (need_initialize_error) {
//                return;
//            }
//            //Timer time;
//            const auto &params = notify.params;
//            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
//            if (ShouldIgnoreFileForIndexing(path))
//                return;
////				work_space_mgr.OnSave(params.textDocument);
//            // 通知消失了
//        });
//
////        _sp.registerHandler([&](Notify_WorkspaceDidChangeWatchedFiles::notify &notify) {
////            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeWatchedFiles");
////        });
////        _sp.registerHandler([&](Notify_WorkspaceDidChangeConfiguration::notify &notify) {
////            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeConfiguration");
//////				do
//////				{
//////					map<std::string, lsp::Any> settings;
//////					try
//////					{
//////						notify.params.settings.Get(settings);
//////					}
//////					catch (...)
//////					{
//////						break;
//////					}
//////
//////				} while (false);
////        });
////        _sp.registerHandler([&](Notify_WorkspaceDidChangeWorkspaceFolders::notify &notify) {
////            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeWorkspaceFolders");
////            if (need_initialize_error) {
////                return;
////            }
//////				work_space_mgr.OnDidChangeWorkspaceFolders(notify.params);
////        });
////
//        _sp.registerHandler([&](const td_shutdown::request &notify) {
//            _log.log(lsp::Log::Level::INFO, "td_shutdown");
//            manager.clearAllStoredContents();
//            td_shutdown::response rsp;
//            on_exit();
//            return rsp;
//        });
//        _sp.registerHandler([&](Notify_Exit::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_Exit");
//        });
//
//        std::thread([&]() {
//            server.run();
//        }).detach();
//    }
//
//    ~Server() {
//        server.stop();
//    }
//
//    std::shared_ptr<lsp::ProtocolJsonHandler> protocol_json_handler = std::make_shared<lsp::ProtocolJsonHandler>();
//    DummyLog _log;
//
//    std::shared_ptr<GenericEndpoint> endpoint = std::make_shared<GenericEndpoint>(_log);
//    lsp::TcpServer server;
//    Condition<bool> esc_event;
//
//};

int main(int argc, char *argv[]) {

    if(argc == 0) {
        std::cerr << "[LSP] No executable path specified" << std::endl;
        return 1;
    }

    CmdOptions options;
    CmdOption cmd_data[] = {
            CmdOption("resources", "res", CmdOptionType::SingleValue, "path to the resources directory of the compiler"),
            CmdOption("port", "port", CmdOptionType::SingleValue, "the port at which lsp should run"),
            CmdOption("version", "v", CmdOptionType::NoValue, "get the version"),
            CmdOption("build-lab", CmdOptionType::SingleValue, "build a lab file and report to parent process"),
            CmdOption("cc", CmdOptionType::SubCommand, "run the compiler with the given command"),
            CmdOption("shmName", CmdOptionType::SingleValue, "the shared memory name for reporting"),
    };
    options.register_options(cmd_data, sizeof(cmd_data) / sizeof(CmdOption));
    options.parse_cmd_options(argc, argv, 1);

    if(options.has_value("version")) {
        std::cout << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << std::endl;
        return 0;
    }

    auto& compileOpt = options.cmd_opt("cc");
    if(compileOpt.has_multi_value()) {
        std::vector<std::string> subc;
        subc.emplace_back(argv[0]);
        compileOpt.get_multi_value_vec(subc);
        auto& command_args = subc;
        char** pointers = convert_to_pointers(command_args);
        // invocation
        auto result = compiler_main(command_args.size(), pointers);
        free_pointers(pointers, command_args.size());
        return result;
    }

    auto build_lab = options.option_new("build-lab");
    if(build_lab.has_value()) {

        ModuleStorage storage;
        const auto context = WorkspaceManager::compile_lab(std::string(argv[0]), std::string(build_lab.value()), storage);

        auto shmName = options.option_new("shmName");
        if(!shmName.has_value() || shmName.value().empty()) {
            std::cerr << "[lsp] expected a 'shmName' command line argument" << std::endl;
            return 1;
        }
        return report_context_to_parent(*context, std::string(shmName.value()));
    }

    std::string user_agent = "websocket-server-async";
    std::string port = "5007";
    {
        auto port_opt = options.option_new("port");
        if(port_opt.has_value()) {
            port = port_opt.value();
        }
    }

    int port_int;
    try {
        port_int = std::atoi(port.data());
    } catch(...) {
        std::cerr << "wrong port " << port << std::endl;
        return 1;
    }

    std::cout << "[LSP] Listening on port " << port << std::endl;

    std::atomic<bool> g_shutdown{false};
    auto socketListener = lsp::io::SocketListener(port_int);

    try {

        while(socketListener.isReady()) {
            auto socket = socketListener.listen();

            if(!socket.isOpen()) {
                std::cout << "[LSP] Socket Not Open" << std::endl;
                break;
            }

            std::cout << "[LSP] Accepted connection"  << std::endl;

            // Detach a thread to serve this client
            std::thread(&run_session, std::ref(g_shutdown), std::ref(socketListener), std::move(socket), argv[0]).detach();

        }

    } catch (const std::exception& e) {
        std::cerr << "[LSP][Fatal] " << e.what() << "\n";
        return 1;
    }

    std::cout << "[LSP] Server shutting down." << std::endl;

    return 0;

}


