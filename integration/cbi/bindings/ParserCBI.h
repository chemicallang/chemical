// Copyright (c) Qinetik 2024.

#pragma once

class ASTAllocator;

class Parser;

class Token;

class ASTNode;

class FunctionType;

class LoopASTNode;

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



}