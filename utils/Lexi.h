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

/**
 * benchmark lexing the given input stream
 * It will print helpful messages like lexing started and time taken by lexing in milli, mico and nano seconds
 * @param file
 * @return tokens
 */
std::vector<std::unique_ptr<LexToken>> benchLexFile(std::istream &file) {

    StreamSourceProvider reader(file);
    Lexer lexer(reader);

    // Print started
    std::cout << "[Lex] Started" << '\n';

    // Save start time
    auto start = std::chrono::steady_clock::now();

    // Actual lexing
    auto lexed = lexer.lex();

    // Save end time
    auto end = std::chrono::steady_clock::now();

    // Calculating duration in different units
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    // Printing stats
    std::cout << "[Lex] Completed " << "(Tokens:" << lexed.size() << ")" << ' ';
    std::cout << "[Lex]:Nanoseconds:" << nanos << "]";
    std::cout << "[Lex]:Microseconds:" << micros << "]";
    std::cout << "[Lex]:Milliseconds:" << millis << "]" << '\n';

    return lexed;

}

/**
 * same as benchLexFile with istream
 * benchmark lexing the filename (relative to in the current project)
 * @param file
 * @return the tokens
 */
std::vector<std::unique_ptr<LexToken>> benchLexFile(const std::string &fileName) {
    std::ifstream file;
    file.open(fileName);
    if (!file.is_open()) {
        std::cerr << "Unknown error opening the file" << '\n';
    }
    auto lexed = benchLexFile(file);
    file.close();
    return lexed;
}

/**
 * will lex the file from given istream
 * @param file
 * @return the tokens
 */
std::vector<std::unique_ptr<LexToken>> lexFile(std::istream& file) {
    StreamSourceProvider reader(file);
    Lexer lexer(reader);
    return lexer.lex();
}

/**
 * same as lexFile with istream
 * lex the path (relative to in the current project)
 * @param fileName
 * @return the tokens
 */
std::vector<std::unique_ptr<LexToken>> lexFile(const std::string &path) {
    std::ifstream file;
    file.open(path);
    if (!file.is_open()) {
        std::cerr << "Unknown error opening the file" << '\n';
    }
    auto lexed = lexFile(file);
    file.close();
    return lexed;
}

#endif //CHEMICALVS_LEXI_H
