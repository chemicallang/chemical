
// Copyright (c) Qinetik 2024.

#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include "LibLsp/lsp/general/initialize.h"
#include "LibLsp/lsp/ProtocolJsonHandler.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/JsonRpc/Condition.h"
#include "LibLsp/lsp/textDocument/did_change.h"
#include "LibLsp/lsp/textDocument/did_save.h"
#include "LibLsp/JsonRpc/Endpoint.h"
#include "LibLsp/JsonRpc/TcpServer.h"
#include "LibLsp/JsonRpc/WebSocketServer.h"
#include "LibLsp/lsp/textDocument/document_symbol.h"
#include "LibLsp/lsp/textDocument/document_link.h"
#include "LibLsp/lsp/textDocument/references.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include <boost/program_options.hpp>
#include <iostream>

#include "LibLsp/lsp/ClientPreferences.h"
#include "LibLsp/lsp/workspace/didChangeWorkspaceFolders.h"
#include "LibLsp/lsp/textDocument/hover.h"
#include "LibLsp/lsp/textDocument/completion.h"

#include "LibLsp/lsp/utils.h"
#include "LibLsp/lsp/working_files.h"
#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "LibLsp/lsp/ParentProcessWatcher.h"
#include "LibLsp/lsp/textDocument/resolveCompletionItem.h"
#include "LibLsp/lsp/textDocument/formatting.h"
#include "LibLsp/lsp/textDocument/documentColor.h"
#include "LibLsp/lsp/general/shutdown.h"
#include "LibLsp/lsp/general/exit.h"
#include "LibLsp/lsp/workspace/did_change_watched_files.h"
#include "LibLsp/lsp/general/initialized.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "LibLsp/lsp/textDocument/rename.h"
#include "LibLsp/lsp/lsAny.h"
#include "LibLsp/lsp/workspace/did_change_configuration.h"
#include "LibLsp/lsp/client/registerCapability.h"
#include "LibLsp/lsp/workspace/symbol.h"
#include "server/WorkspaceManager.h"
#include "utils/FileUtils.h"
#include "utils/CmdUtils.h"

using namespace boost::asio::ip;
using namespace std;
using namespace lsp;


#include <thread>
#include <atomic>
#include <functional>
#include <boost/asio.hpp>
#include "utils/JsonUtils.h"

class DummyLog : public lsp::Log {
public:

    void log(Level level, std::wstring &&msg) {
        std::wcout << msg << std::endl;
    };

    void log(Level level, const std::wstring &msg) {
        std::wcout << msg << std::endl;
    };

    void log(Level level, std::string &&msg) {
        std::cout << msg << std::endl;
    };

    void log(Level level, const std::string &msg) {
        std::cout << msg << std::endl;
    };
};

std::string _address = "127.0.0.1";

bool ShouldIgnoreFileForIndexing(const std::string &path) {
    return StartsWith(path, "git:");
}

class Server {
public:

    RemoteEndPoint &_sp;
    WorkspaceManager manager;
    boost::optional<Rsp_Error> need_initialize_error;
    std::unique_ptr<ParentProcessWatcher> parent_process_watcher;
    std::shared_ptr<ClientPreferences> clientPreferences;

    struct ExitMsgMonitor {
        std::atomic_bool is_running_ = true;

        bool isCancelled() {
            return !is_running_.load(std::memory_order_relaxed);
        }

        void Cancel() {
            is_running_.store(false, std::memory_order_relaxed);
        }
    };

    ExitMsgMonitor exit_monitor;

    void on_exit() {
        exit_monitor.Cancel();
        server.stop();
        esc_event.notify(std::make_unique<bool>(true));
    }

    std::map<std::string, Registration> registeredCapabilities;

    void collectRegisterCapability(const std::string &method) {
        if (registeredCapabilities.find(method) == registeredCapabilities.end()) {
            auto reg = Registration::Create(method);
            registeredCapabilities[method] = reg;
        }
    }

    Server(
            const std::string &user_agent,
            const std::string &_port,
            bool _enable_watch_parent_process,
            std::string lsp_exe_path
    ) : _sp(server.point), server(_address, _port, protocol_json_handler, endpoint, _log), manager(lsp_exe_path) {

        manager.remote = &server.point;
        need_initialize_error = Rsp_Error();
        need_initialize_error->error.code = lsErrorCodes::ServerNotInitialized;
        need_initialize_error->error.message = "Server is not initialized";

        _sp.registerHandler([=](const td_initialize::request &req) {

            _log.log(lsp::Log::Level::INFO, "td_initialize");

            need_initialize_error.reset();
            clientPreferences = std::make_shared<ClientPreferences>(req.params.capabilities);
            td_initialize::response rsp;
            lsServerCapabilities capabilities;
            auto SETTINGS_KEY = "settings";

            if (req.params.initializationOptions) {
                do {
                    map<std::string, lsp::Any> initializationOptions;
                    try {
                        lsp::Any init_object = req.params.initializationOptions.value();

                        init_object.Get(initializationOptions);
                    }
                    catch (...) {
                        break;
                    }
                    map<std::string, lsp::Any> settings;
                    try {

                        initializationOptions[SETTINGS_KEY].Get(settings);
                    }
                    catch (...) {
                        break;
                    }

                } while (false);
            }


            std::pair<optional<lsTextDocumentSyncKind>, optional<lsTextDocumentSyncOptions>> textDocumentSync;
            lsTextDocumentSyncOptions options;
            options.openClose = true;
            options.change = lsTextDocumentSyncKind::Incremental;
            options.willSave = false;
            options.willSaveWaitUntil = false;
            textDocumentSync.second = options;
            capabilities.textDocumentSync = textDocumentSync;

			    if(!clientPreferences->isHoverDynamicRegistered())
			    {
					capabilities.hoverProvider = true;
			    }
				if (!clientPreferences->isCompletionDynamicRegistered())
				{
					lsCompletionOptions completion;
					completion.resolveProvider = true;
					capabilities.completionProvider = completion;
				}
				std::pair< optional<bool>, optional<WorkDoneProgressOptions> > option;
				option.first = true;

				if (!clientPreferences->isDefinitionDynamicRegistered())
				{
					capabilities.definitionProvider = { true, std::nullopt };
				}
				if (!clientPreferences->isFoldgingRangeDynamicRegistered())
				{
					capabilities.foldingRangeProvider = std::pair< optional<bool>, optional<FoldingRangeOptions> >();
					capabilities.foldingRangeProvider->first = true;
				}
//				if (!clientPreferences->isReferencesDynamicRegistered())
//				{
//					capabilities.referencesProvider = option;
//				}
				if (!clientPreferences->isDocumentSymbolDynamicRegistered())
				{
					capabilities.documentSymbolProvider = option;
				}
//				if (!clientPreferences->isFormattingDynamicRegistrationSupported())
//				{
//					capabilities.documentFormattingProvider = option;
//				}
//				if (!clientPreferences->isRenameDynamicRegistrationSupported())
//				{
//					std::pair< boost::optional<bool>, boost::optional<RenameOptions> > rename_opt;
//					rename_opt.first = true;
//					capabilities.renameProvider = rename_opt;
//				}

//            {

            SemanticTokensWithRegistrationOptions semantic_tokens_opt;
            auto semanticTokenTypes = [] {
                std::vector<std::string> _type;
                for (unsigned i = 0; i <= static_cast<unsigned>(SemanticTokenType::lastKind);
                     ++i)
                    _type.push_back(to_string(static_cast<SemanticTokenType>(i)));
                return _type;
            };
            auto semanticTokenModifiers = [] {
                std::vector<std::string> _type;
                for (unsigned i = 0; i <= static_cast<unsigned>(SemanticTokenModifier::LastModifier);
                     ++i)
                    _type.push_back(to_string(static_cast<SemanticTokenModifier>(i)));
                return _type;
            };

            semantic_tokens_opt.legend.tokenTypes = semanticTokenTypes();
            semantic_tokens_opt.legend.tokenModifiers = semanticTokenModifiers();

            std::pair<optional<bool>, optional<lsp::Any> > rang;
            rang.first = false;
            semantic_tokens_opt.range = rang;

            std::pair<optional<bool>,
                    optional<SemanticTokensServerFull> > full;
            full.first = true;

            semantic_tokens_opt.full = full;
            capabilities.semanticTokensProvider = std::move(semantic_tokens_opt);
//            }

            rsp.result.capabilities.swap(capabilities);
            WorkspaceServerCapabilities workspace_server_capabilities;
            //capabilities.workspace
            if (req.params.processId.has_value() && _enable_watch_parent_process) {
                parent_process_watcher = std::make_unique<ParentProcessWatcher>(_log, req.params.processId.value(),
                                                                                [&]() {
                                                                                    on_exit();
                                                                                });
            }

//            _log.log(lsp::Log::Level::INFO, rsp.ToJson());

            return std::move(rsp);
        });
        _sp.registerHandler([&](Notify_InitializedNotification::notify &notify) {

            _log.log(lsp::Log::Level::INFO, "Notify_InitializedNotification");

            if (!clientPreferences)return;

            _log.log(lsp::Log::Level::INFO, "Notify_InitializedNotification ClientPrefs Set");

            if (clientPreferences->isCompletionDynamicRegistered()) {
                collectRegisterCapability(td_completion::request::kMethodInfo);
            }
            if (clientPreferences->isWorkspaceSymbolDynamicRegistered()) {
                collectRegisterCapability(wp_symbol::request::kMethodInfo);
            }
            if (clientPreferences->isDocumentSymbolDynamicRegistered()) {
                collectRegisterCapability(td_symbol::request::kMethodInfo);
            }
            if(clientPreferences->isDocumentLinkDynamicRegistered()) {
                  collectRegisterCapability(td_links::request::kMethodInfo);
            }
            /*if (clientPreferences->isCodeActionDynamicRegistered())
            {
                collectRegisterCapability(td_codeAction::request::kMethodInfo);
            }*/
            if (clientPreferences->isDefinitionDynamicRegistered()) {
                collectRegisterCapability(td_definition::request::kMethodInfo);
            }
            if (clientPreferences->isHoverDynamicRegistered()) {
                collectRegisterCapability(td_hover::request::kMethodInfo);
            }
            if (clientPreferences->isReferencesDynamicRegistered()) {
                collectRegisterCapability(td_references::request::kMethodInfo);
            }
            if (clientPreferences->isFoldgingRangeDynamicRegistered()) {
                collectRegisterCapability(td_foldingRange::request::kMethodInfo);
            }
            if (clientPreferences->isWorkspaceFoldersSupported()) {
                collectRegisterCapability(Notify_WorkspaceDidChangeWorkspaceFolders::notify::kMethodInfo);
            }
            if (clientPreferences->isWorkspaceDidChangeConfigurationSupported()) {
                collectRegisterCapability(Notify_WorkspaceDidChangeConfiguration::notify::kMethodInfo);
            }
            if (clientPreferences->isSemanticHighlightingSupported()) {
                collectRegisterCapability(td_semanticTokens_full::request::kMethodInfo);
            }

            Req_ClientRegisterCapability::request request;
            for (auto &it: registeredCapabilities) {
                request.params.registrations.push_back(it.second);
            }
            _sp.send(request);

        });
        _sp.registerHandler(
                [&](const td_definition::request &req, const CancelMonitor &monitor)
                        -> lsp::ResponseOrError<td_definition::response> {
                    _log.log(lsp::Log::Level::INFO, "td_definition");
                    if (need_initialize_error) {
                        return need_initialize_error.value();
                    }
                return manager.get_definition(req.params.textDocument.uri, req.params.position);;
        });
        _sp.registerHandler([&](const td_symbol::request &req) -> lsp::ResponseOrError<td_symbol::response> {
            _log.log(lsp::Log::Level::INFO, "td_symbol");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            return manager.get_symbols(req.params.textDocument.uri);
        });
        _sp.registerHandler(
                [&](const td_hover::request &req, const CancelMonitor &monitor)
                        -> lsp::ResponseOrError<td_hover::response> {
                    _log.log(lsp::Log::Level::INFO, "td_hover");
                    if (need_initialize_error) {
                        return need_initialize_error.value();
                    }
                    return manager.get_hover(req.params.textDocument.uri, req.params.position);
                });
        _sp.registerHandler([&](const td_completion::request &req, const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_completion::response> {
            _log.log(lsp::Log::Level::INFO, "td_completion");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            return manager.get_completion(req.params.textDocument.uri, req.params.position.line, req.params.position.character);
        });
        _sp.registerHandler([&](const completionItem_resolve::request &req) {
            _log.log(lsp::Log::Level::INFO, "completionItem_resolve");
            completionItem_resolve::response rsp;
            rsp.result = req.params;
            return std::move(rsp);
        });

        _sp.registerHandler([&](const td_foldingRange::request &req,
                                const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_foldingRange::response> {
            _log.log(lsp::Log::Level::INFO, "td_foldingRange");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            return manager.get_folding_range(req.params.textDocument.uri);
        });
        _sp.registerHandler([&](const td_formatting::request &req,
                                const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_formatting::response> {
            _log.log(lsp::Log::Level::INFO, "td_formatting");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            td_formatting::response rsp;
//				auto unit = GetUnit(req.params.textDocument);
//				if (unit){
//					DocumentFormatHandler(unit, rsp.result, req.params.options);
//				}
            return std::move(rsp);
        });
        _sp.registerHandler([&](const td_links::request &req,
                                const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_links::response> {
            _log.log(lsp::Log::Level::INFO, "td_links");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            return manager.get_links(req.params.textDocument.uri);
        });
        _sp.registerHandler([&](const td_linkResolve::request &req,
                                const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_linkResolve::response> {
            _log.log(lsp::Log::Level::INFO, "td_linkResolve");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            // No need to support this request, since we send resolved links to the IDE
            td_linkResolve::response rsp;
            return std::move(rsp);
        });
//        _sp.registerHandler([&](const td_documentColor::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_documentColor::response> {
//            _log.log(lsp::Log::Level::INFO, "td_documentColor");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            td_documentColor::response rsp;
//
////				auto unit = GetUnit(req.params.textDocument);
////				if (unit){
////					DocumentColorHandler(unit, rsp.result);
////				}
//            return std::move(rsp);
//        });
        auto x = 0;
        _sp.registerHandler([&](const td_semanticTokens_full::request &req,
                                const CancelMonitor &monitor)
                                    -> lsp::ResponseOrError<td_semanticTokens_full::response> {
            _log.log(lsp::Log::Level::INFO, "td_semanticTokens_full");
            if (need_initialize_error) {
                return need_initialize_error.value();
            }
            return manager.get_semantic_tokens_full(req.params.textDocument.uri);
        });
//        _sp.registerHandler([&](const td_references::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_references::response> {
//            _log.log(lsp::Log::Level::INFO, "td_references");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            td_references::response rsp;
////				auto unit = GetUnit(req.params.textDocument);
////				if (unit){
////					ReferencesHandler(unit, req.params.pos, rsp.result,&_requestMonitor);
////				}
//            return std::move(rsp);
//        });
//        _sp.registerHandler([&](const td_rename::request &req,
//                                const CancelMonitor &monitor)
//                                    -> lsp::ResponseOrError<td_rename::response> {
//            _log.log(lsp::Log::Level::INFO, "td_rename");
//            if (need_initialize_error) {
//                return need_initialize_error.value();
//            }
//            td_rename::response rsp;
////				auto unit = GetUnit(req.params.textDocument);
////				if (unit) {
//            std::vector<lsWorkspaceEdit::Either> edits;
////					RenameHandler(unit, req.params, edits, &_requestMonitor);
//            rsp.result.documentChanges = std::move(edits);
//
////				}
//            return std::move(rsp);
//        });
        _sp.registerHandler([&](Notify_TextDocumentDidOpen::notify &notify) {

            _log.log(lsp::Log::Level::INFO, "TextDocumentDidOpen Received");

            if (need_initialize_error) {
                return;
            }

            _log.log(lsp::Log::Level::INFO, "TextDocumentDidOpen Received 2");

            auto &params = notify.params;
            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
            if (ShouldIgnoreFileForIndexing(path))
                return;
//				work_space_mgr.OnOpen(params.textDocument);
        });

        _sp.registerHandler([&](Notify_TextDocumentDidChange::notify &notify) {

            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received 1");

            if (need_initialize_error) {
                return;
            }

            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received\n");

            // to json the request
//            _log.log(lsp::Log::Level::INFO, notify.ToJson());
//            _log.log(lsp::Log::Level::INFO, "\nAbove was the json");
            const auto &params = notify.params;
            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();

            if(params.contentChanges.empty()) {
                _log.log(lsp::Log::Level::INFO, "No Changes in the code");
            }

            manager.onChangedContents(params.textDocument.uri, params.contentChanges);

            _log.log(lsp::Log::Level::INFO, "TextDocumentDidChange Received 3");

        });

        _sp.registerHandler([&](Notify_TextDocumentDidClose::notify &notify) {
            _log.log(lsp::Log::Level::INFO, "Notify_TextDocumentDidClose");

            if (need_initialize_error) {
                return;
            }
            //Timer time;
            const auto &params = notify.params;
            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
            if (ShouldIgnoreFileForIndexing(path))
                return;

            manager.onClosedFile(path.path);

        });

        _sp.registerHandler([&](Notify_TextDocumentDidSave::notify &notify) {
            _log.log(lsp::Log::Level::INFO, "Notify_TextDocumentDidSave");
            if (need_initialize_error) {
                return;
            }
            //Timer time;
            const auto &params = notify.params;
            AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
            if (ShouldIgnoreFileForIndexing(path))
                return;
//				work_space_mgr.OnSave(params.textDocument);
            // 通知消失了
        });

//        _sp.registerHandler([&](Notify_WorkspaceDidChangeWatchedFiles::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeWatchedFiles");
//        });
//        _sp.registerHandler([&](Notify_WorkspaceDidChangeConfiguration::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeConfiguration");
////				do
////				{
////					map<std::string, lsp::Any> settings;
////					try
////					{
////						notify.params.settings.Get(settings);
////					}
////					catch (...)
////					{
////						break;
////					}
////
////				} while (false);
//        });
//        _sp.registerHandler([&](Notify_WorkspaceDidChangeWorkspaceFolders::notify &notify) {
//            _log.log(lsp::Log::Level::INFO, "Notify_WorkspaceDidChangeWorkspaceFolders");
//            if (need_initialize_error) {
//                return;
//            }
////				work_space_mgr.OnDidChangeWorkspaceFolders(notify.params);
//        });
//
        _sp.registerHandler([&](const td_shutdown::request &notify) {
            _log.log(lsp::Log::Level::INFO, "td_shutdown");
            manager.clearAllStoredContents();
            td_shutdown::response rsp;
            return rsp;
        });
        _sp.registerHandler([&](Notify_Exit::notify &notify) {
            _log.log(lsp::Log::Level::INFO, "Notify_Exit");
        });

        std::thread([&]() {
            server.run();
        }).detach();
    }

    ~Server() {
        server.stop();
    }

    std::shared_ptr<lsp::ProtocolJsonHandler> protocol_json_handler = std::make_shared<lsp::ProtocolJsonHandler>();
    DummyLog _log;

    std::shared_ptr<GenericEndpoint> endpoint = std::make_shared<GenericEndpoint>(_log);
    lsp::TcpServer server;
    Condition<bool> esc_event;

};

const char VERSION[] = "LPG-language-server 0.2.3 (" __DATE__ ")";

const char *_PORT_STR = "port";

int libtcc_test();

int main(int argc, char *argv[]) {

    libtcc_test();

    if(argc == 0) {
        std::cerr << "[LSP] No executable path specified" << std::endl;
        return 1;
    }

    CmdOptions options;
    auto args = options.parse_cmd_options(argc, argv, 1);

//    using namespace boost::program_options;
//    options_description desc(" LPG-language-server allowed options");
//    desc.add_options()
//            (_PORT_STR, value<int>(), "tcp port")
//            ("watchParentProcess", "enable watch parent process")
//            ("help,h", "produce help message")
//            ("version,v", VERSION);

//    variables_map vm;
//    try {
//        store(parse_command_line(argc, argv, desc), vm);
//    }
//    catch (std::exception &e) {
//        std::cout << "Undefined input.Reason:" << e.what() << std::endl;
//        return 0;
//    }
//    notify(vm);


//    if (vm.count("help") || vm.count("h")) {
//        cout << desc << endl;
//        return 1;
//    }
//    if (vm.count("version") || vm.count("v")) {
//        cout << VERSION << endl;
//        return 1;
//    }
    bool enable_watch_parent_process = false;
    if (options.option("watch-parent-process", "wpp").has_value()) {
        enable_watch_parent_process = true;
    }
    std::string user_agent = std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async";
    Server server(user_agent, "5007", enable_watch_parent_process, argv[0]);

    auto resources_path = options.option_e("resources", "res");
    if(!resources_path.empty()) {
        server.manager.overridden_resources_path = resources_path;
    }

    auto ret = server.esc_event.wait();
    if (ret) {
        return 0;
    }
    return -1;
}


