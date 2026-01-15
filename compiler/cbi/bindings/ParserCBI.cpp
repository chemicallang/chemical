// Copyright (c) Chemical Language Foundation 2025.

#include "ParserCBI.h"
#include "parser/Parser.h"
#include "compiler/cbi/model/ASTBuilder.h"

Token** ParsergetTokenPtr(Parser* parser) {
    return &parser->token;
}

uint64_t ParsergetEncodedLocation(Parser* parser, Token* token) {
    return parser->loc_single(token);
}

AnnotationController* ParsergetAnnotationController(Parser* parser) {
    return &parser->controller;
}

bool ParsergetIs64Bit(Parser* parser) {
    return parser->is64Bit;
}

ASTNode** ParsergetParentNodePtr(Parser* parser) {
    return &parser->parent_node;
}

void ParsergetCurrentFilePath(chem::string_view* view, Parser* parser) {
    *view = chem::string_view(parser->stored_file_path.data(), parser->stored_file_path.size());
}

Value* ParserparseExpression(Parser* parser, ASTBuilder* builder, bool parseStruct, bool parseLambda) {
    return parser->parseExpression(*builder->allocator, parseStruct, parseLambda);
}

ASTNode* ParserparseNestedLevelStatement(Parser* parser, ASTBuilder* builder) {
    return parser->parseNestedLevelStatementTokens(*builder->allocator, false, false);
}

void Parsererror_at(Parser* parser, chem::string_view* view, Token* token) {
    parser->error(*view, token->position);
}