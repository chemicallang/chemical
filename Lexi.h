//
// Created by wakaz on 26/01/2024.
//

#ifndef CHEMICALVS_LEXI_H
#define CHEMICALVS_LEXI_H

#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include "SourceProvider.cpp"
#include "lexer/Lexer.h"

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

#endif //CHEMICALVS_LEXI_H
