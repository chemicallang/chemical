// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::collect_cbi_node(unsigned int start, unsigned int end) {
    if(!binder) return false;
    if(current_cbi.empty()) {
        error("cannot collect into empty 'cbi', use cbi:begin('name') or cbi:to('name')");
        return false;
    }
    if(end - start <= 0) return false;
    std::vector<CSTToken*> copied_ptrs;
    copied_ptrs.reserve(end - start);
    unsigned int current = start;
    // copy the pointers only
    while(current < end) {
        copied_ptrs.emplace_back(unit.tokens[current]);
        current++;
    }
    // compile
    binder->collect(current_cbi, copied_ptrs, !isCBICollectingGlobal);
    return true;
}