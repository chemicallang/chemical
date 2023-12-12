#include <iostream>
#include <fstream>
#include "SourceProvider.cpp"
#include "lexer/Lexer.h"
#include <chrono>

int main() {

    std::ifstream file;

    file.open("file.txt");

    if (!file.is_open()) {
        std::cerr << "error opening a file" << '\n';
        return 1;
    }

    StreamSourceProvider reader(file);
    Lexer lexer(reader);

//    std::cout << "Lex Start " << '\n';

    auto start = std::chrono::steady_clock::now();

    auto lexed = lexer.lex();

    auto end = std::chrono::steady_clock::now();

    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end-start).count();

    std::cout << "Lex Complete " << "[Size:" << lexed.size() << "]" << ' ';
    std::cout << "[Nanoseconds:" << nanos << "]";
    std::cout << "[Microseconds:" << micros << "]";
    std::cout << "[Milliseconds:" << millis << "]" << '\n';

    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start << "," << item->end << ")";
        if(!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << '\n';
    }

    file.close();

    return 0;
}
