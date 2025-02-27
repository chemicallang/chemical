// Copyright (c) Chemical Language Foundation 2025.

#include "ParserCBI.h"
#include "parser/Parser.h"

Token** ParsergetTokenPtr(Parser* parser) {
    return &parser->token;
}

ASTAllocator* ParsergetGlobalBuilder(Parser* parser) {
    return &parser->global_allocator;
}

ASTAllocator* ParsergetModuleBuilder(Parser* parser) {
    return &parser->mod_allocator;
}

bool ParsergetIs64Bit(Parser* parser) {
    return parser->is64Bit;
}

ASTNode** ParsergetParentNodePtr(Parser* parser) {
    return &parser->parent_node;
}

FunctionType** ParsergetCurrentFuncTypePtr(Parser* parser) {
    return &parser->current_func_type;
}

LoopASTNode** ParsergetCurrentLoopNodePtr(Parser* parser) {
    return &parser->current_loop_node;
}

void ParsergetCurrentFilePath(chem::string_view* view, Parser* parser) {
    *view = chem::string_view(parser->stored_file_path.data(), parser->stored_file_path.size());
}

Value* ParserparseExpression(Parser* parser, ASTAllocator* allocator, bool parseStruct, bool parseLambda) {
    return parser->parseExpression(*allocator, parseStruct, parseLambda);
}

void Parsererror_at(Parser* parser, chem::string_view* view, Token* token) {
    parser->error(*view, token->position);
}