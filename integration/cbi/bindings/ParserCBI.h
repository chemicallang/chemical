// Copyright (c) Qinetik 2024.

#pragma once

class ASTAllocator;

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

    ASTAllocator* ParsergetGlobalBuilder(Parser* parser);

    ASTAllocator* ParsergetModuleBuilder(Parser* parser);

    bool ParsergetIs64Bit(Parser* parser);

    ASTNode** ParsergetParentNodePtr(Parser* parser);

    FunctionType** ParsergetCurrentFuncTypePtr(Parser* parser);

    LoopASTNode** ParsergetCurrentLoopNodePtr(Parser* parser);

    void ParsergetCurrentFilePath(chem::string_view* view, Parser* parser);

    Value* ParserparseExpression(Parser* parser, ASTAllocator* allocator,  bool parseStruct, bool parseLambda);

    void Parsererror_at(Parser* parser, chem::string_view* view, Token* token);


}