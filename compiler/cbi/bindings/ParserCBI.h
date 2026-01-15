// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "cstdint"

class ASTBuilder;

class ASTAllocator;

class AnnotationController;

class Parser;

class Token;

class ASTNode;

class FunctionType;

class LoopASTNode;

class Value;

namespace chem {
    class string_view;
}

extern "C" {

    Token** ParsergetTokenPtr(Parser* parser);

    uint64_t ParsergetEncodedLocation(Parser* parser, Token* token);

    AnnotationController* ParsergetAnnotationController(Parser* parser);

    bool ParsergetIs64Bit(Parser* parser);

    ASTNode** ParsergetParentNodePtr(Parser* parser);

    void ParsergetCurrentFilePath(chem::string_view* view, Parser* parser);

    Value* ParserparseExpression(Parser* parser, ASTBuilder* builder,  bool parseStruct, bool parseLambda);

    ASTNode* ParserparseNestedLevelStatement(Parser* parser, ASTBuilder* builder);

    void Parsererror_at(Parser* parser, chem::string_view* view, Token* token);


}