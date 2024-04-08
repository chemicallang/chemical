// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"
#include "parser/Parser.h"

bool Lexer::collectStructAsLexer(unsigned int start, unsigned int end) {
    if(has_errors) {
        error("please resolve errors above before declaring a struct for collection");
        return false;
    }
    if(end - start <= 0) return false;
    std::vector<std::unique_ptr<CSTToken>> copied_ptrs;
    copied_ptrs.reserve(end - start);
    unsigned int current = start;
    // copy the pointers only
    while(current < end) {
        copied_ptrs.emplace_back(tokens[current].get());
        current++;
    }
    return false;
    // TODO CST needs to be converted to AST
    // create parser, and parse struct
//    Parser parser(std::move(copied_ptrs));
//    parser.isParseInterpretableExpressions = true;
//    auto definition = parser.parseStructDefinition();
//    // release all the pointers, so tokens don't get deleted
//    for(auto& ptr : parser.tokens) {
//        ptr.release();
//    }
//    if(parser.errors.empty()){
//        if(definition.has_value()) {
//            if(isLexCompTimeLexer) {
//                lexer_structs[definition.value()->name] = std::move(definition.value());
//                isLexCompTimeLexer = false;
//            } else {
//                definition.value()->interpret(interpret_scope);
//                collected[definition.value()->name] = std::move(definition.value());
//            }
//        } else {
//            error("couldn't find struct");
//            return false;
//        }
//    } else {
//        for(auto& err : parser.errors) {
//            errors.push_back(std::move(err));
//        }
//    }
//    return true;
}