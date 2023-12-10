#include <iostream>
#include <fstream>
#include "SourceProvider.cpp"
#include "lexer/Lexer.h"

int main() {

    std::ifstream file;

    file.open("file.txt");

    if (!file.is_open()) {
        std::cerr << "error opening a file" << std::endl;
        return 1;
    }

    StreamSourceProvider reader(file);
    Lexer lexer(reader);

//    std::cout << "Lex Start " << std::endl;

    auto lexed = lexer.lex();

    std::cout << "Lex Complete " << "[Size:" << lexed.size() << "]" << std::endl;

    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start << "," << item->end << ")";
        if(!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << std::endl;
    }

    file.close();

    return 0;
}
