#include <iostream>
#include <fstream>
#include "SourceProvider.cpp"
#include "lexer/Lexer.h"
#include <chrono>
#include "LibLsp/JsonRpc/WebSocketServer.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "LibLsp/lsp/general/initialize.h"
#include "LibLsp/lsp/ProtocolJsonHandler.h"
#include "LibLsp/lsp/textDocument/typeHierarchy.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/resolveCompletionItem.h"


#include "LibLsp/JsonRpc/Endpoint.h"
#include "LibLsp/JsonRpc/stream.h"
#include "LibLsp/JsonRpc/TcpServer.h"
#include "LibLsp/lsp/textDocument/document_symbol.h"
#include "LibLsp/lsp/workspace/execute_command.h"

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include "LibLsp/JsonRpc/Endpoint.h"
#include "LibLsp/JsonRpc/RemoteEndPoint.h"
#include "LibLsp/JsonRpc/stream.h"
#include "LibLsp/lsp/ProtocolJsonHandler.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::string _address = "127.0.0.1";
std::string _port = "9333";

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

class Server {
public:
    Server(const std::string &user_agent) : server(user_agent, _address, _port, protocol_json_handler, endpoint, _log) {
        server.point.registerHandler(
                [&](const td_initialize::request &req) {
                    td_initialize::response rsp;
                    CodeLensOptions code_lens_options;
                    code_lens_options.resolveProvider = true;
                    rsp.result.capabilities.codeLensProvider = code_lens_options;
                    return rsp;
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
    lsp::WebSocketServer server;

};

void testLexerOnFile(std::string fileName = "file.txt") {


    std::ifstream file;

    file.open(fileName);

    if (!file.is_open()) {
        std::cerr << "error opening a file" << '\n';
        return;
    }

    StreamSourceProvider reader(file);
    Lexer lexer(reader);

//    std::cout << "Lex Start " << '\n';

    auto start = std::chrono::steady_clock::now();

    auto lexed = lexer.lex();

    auto end = std::chrono::steady_clock::now();

    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    std::cout << "Lex Complete " << "[Size:" << lexed.size() << "]" << ' ';
    std::cout << "[Nanoseconds:" << nanos << "]";
    std::cout << "[Microseconds:" << micros << "]";
    std::cout << "[Milliseconds:" << millis << "]" << '\n';

    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start << "," << item->end << ")";
        if (!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << '\n';
    }

    file.close();

}

int main() {
    std::string user_agent = std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async";
    Server server(user_agent);
    return 0;
}
