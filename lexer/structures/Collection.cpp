// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::collect_cbi_node(unsigned int start, unsigned int end) {
    if(!isCBIEnabled) return false;
    if(end - start <= 0) return false;
    std::vector<std::unique_ptr<CSTToken>> copied_ptrs;
    copied_ptrs.reserve(end - start);
    unsigned int current = start;
    // copy the pointers only
    while(current < end) {
        copied_ptrs.emplace_back(tokens[current].get());
        current++;
    }
    // compile
    auto result = binder->compile(copied_ptrs);
    // release all the pointers, so tokens don't get deleted
    for(auto& ptr : copied_ptrs) {
        ptr.release();
    }
    return result;
}