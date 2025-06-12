
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
#include "server/build/ContextSerialization.h"

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

        lsp::CompletionOptions completionOptions;
        std::vector<std::string> completionTriggers = { ".", "::" };
        completionOptions.triggerCharacters = std::move(completionTriggers);

        lsp::SignatureHelpOptions signatureOptions;
        std::vector<std::string> signatureTriggers = { "," };
        signatureOptions.triggerCharacters = std::move(signatureTriggers);

        return lsp::requests::Initialize::Result{
                .capabilities = {
                        .textDocumentSync = textDocSync,
                        .completionProvider = completionOptions,
                        .hoverProvider = true,
                        .signatureHelpProvider = signatureOptions,
                        .definitionProvider = true,
                        .documentSymbolProvider = symbolOptions,
                        .foldingRangeProvider = foldingOptions,
                        .semanticTokensProvider = tokensProvider,
                        .inlayHintProvider = true,
                },
                .serverInfo = lsp::InitializeResultServerInfo{
                        .name    = "Chemical Language Server",
                        .version = versionStr.str()
                }
        };

    });

    handler.add<lsp::requests::TextDocument_SemanticTokens_Full>([&manager](lsp::requests::TextDocument_SemanticTokens_Full::Params&& params){
        try {
            auto tokens = manager.get_semantic_tokens_full(params.textDocument.uri.path());
            return lsp::requests::TextDocument_SemanticTokens_Full::Result(lsp::SemanticTokens{
                    .data = tokens
            });
        } catch(const std::exception& e) {
            throw std::runtime_error("UNCAUGHT_EXCEPTION");
        }
    });

    handler.add<lsp::requests::TextDocument_FoldingRange>([&manager](lsp::requests::TextDocument_FoldingRange::Params&& params){
        return lsp::Nullable(manager.get_folding_range(params.textDocument.uri.path()));
    });

    handler.add<lsp::requests::TextDocument_DocumentSymbol>([&manager](lsp::requests::TextDocument_DocumentSymbol::Params&& params){
        return lsp::NullableVariant<std::vector<lsp::SymbolInformation>, std::vector<lsp::DocumentSymbol>>(manager.get_symbols(params.textDocument.uri.path()));
    });

    handler.add<lsp::requests::TextDocument_Hover>([&manager](lsp::requests::TextDocument_Hover::Params&& params){
        auto hoverStr = manager.get_hover(params.textDocument.uri.path(), Position { params.position.line, params.position.character });
        return lsp::NullOr<lsp::Hover>(lsp::Hover{ lsp::MarkupContent{lsp::MarkupKind::Markdown, std::move(hoverStr)} });
    });

    handler.add<lsp::requests::TextDocument_Definition>([&manager](lsp::requests::TextDocument_Definition::Params&& params) -> lsp::TextDocument_DefinitionResult {
        auto& pos = params.position;
        return lsp::TextDocument_DefinitionResult(manager.get_definition(params.textDocument.uri.path(), Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_Completion>([&manager](lsp::requests::TextDocument_Completion::Params&& params) -> lsp::TextDocument_CompletionResult {
        auto& pos = params.position;
        return lsp::TextDocument_CompletionResult(manager.get_completion(params.textDocument.uri.path(), Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_SignatureHelp>([&manager](lsp::requests::TextDocument_SignatureHelp::Params&& params) -> lsp::TextDocument_SignatureHelpResult {
        auto& pos = params.position;
        return lsp::TextDocument_SignatureHelpResult(manager.get_signature_help(params.textDocument.uri.path(), Position { pos.line, pos.character }));
    });

    handler.add<lsp::requests::TextDocument_InlayHint>([&manager](lsp::requests::TextDocument_InlayHint::Params&& params) -> lsp::TextDocument_InlayHintResult {
        auto& start = params.range.start;
        auto& end = params.range.end;
        auto range = Range { Position { start.line, start.character }, Position { end.line, end.character } };
        return lsp::TextDocument_InlayHintResult(manager.get_hints(params.textDocument.uri.path(), range));
    });

    handler.add<lsp::notifications::TextDocument_DidOpen>([&manager](lsp::notifications::TextDocument_DidOpen::Params&& params){
        manager.OnOpenedFile(params.textDocument.uri.path());
    });

    handler.add<lsp::notifications::TextDocument_DidChange>([&manager](lsp::notifications::TextDocument_DidChange::Params&& params){
        manager.onChangedContents(params.textDocument.uri.path(), params.contentChanges);
    });

    handler.add<lsp::notifications::Workspace_DidChangeWatchedFiles>([&manager](lsp::notifications::Workspace_DidChangeWatchedFiles::Params&& params){
        for(auto& change : params.changes) {
            switch(change.type.index()) {
                case lsp::FileChangeType::Created:
                    manager.index_new_file(change.uri.path());
                    break;
                case lsp::FileChangeType::Deleted:
                    manager.de_index_deleted_file(change.uri.path());
                    break;
                case lsp::FileChangeType::Changed:
                default:
                    return;
            }
        }
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
            CmdOption("evtChildDone", CmdOptionType::SingleValue, "the event for when child is done"),
            CmdOption("evtParentAck", CmdOptionType::SingleValue, "the event for parent acknowledgement"),
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

        auto childDone = options.option_new("evtChildDone");
        if(!childDone.has_value() || childDone.value().empty()) {
            std::cerr << "[lsp] expected a 'evtChildDone' command line argument" << std::endl;
            return 1;
        }

        auto parentAck = options.option_new("evtParentAck");
        if(!parentAck.has_value() || parentAck.value().empty()) {
            std::cerr << "[lsp] expected a 'evtParentAck' command line argument" << std::endl;
            return 1;
        }

        return report_context_to_parent(*context, std::string(shmName.value()), std::string(childDone.value()), std::string(parentAck.value()));

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


