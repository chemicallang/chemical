// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "Persi.h"

void benchParse(Parser& parser) {

    // Save start time
    auto start = std::chrono::steady_clock::now();

    // Print started
    // std::cout << "[Parse] Started" << '\n';

    // Actual parsing
    parser.parse();

    // Save end time
    auto end = std::chrono::steady_clock::now();

    // Calculating duration in different units
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    // Printing stats
    std::cout << "[Parse] Completed " << "(Nodes:" << parser.nodes.size() << ")" << ' ';
    std::cout << "[Nanoseconds:" << nanos << "]";
    std::cout << "[Microseconds:" << micros << "]";
    std::cout << "[Milliseconds:" << millis << "]" << '\n';
}


Parser benchParse(std::vector<std::unique_ptr<LexToken>> tokens) {
    Parser parser(std::move(tokens));
    benchParse(parser);
    return parser;
}

Parser parse(std::vector<std::unique_ptr<LexToken>> tokens) {
    Parser parser(std::move(tokens));
    parser.parse();
    return parser;
}