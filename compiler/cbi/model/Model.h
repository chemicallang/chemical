// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class Lexer;

class Token;

class Parser;

class Value;

class ASTNode;

class ASTAllocator;

/**
 * the function that is called to initialize lexer for the user lexer
 */
typedef void(*EmbeddedLexerInitializeFn)(Lexer* lexer);

/**
 * the function that is called to provide the next token, if user lexer is active
 */
typedef void(*EmbeddedLexerGetNextTokenFn)(Token* returning_token, void* instance, Lexer* lexer);

/**
 * The fat pointer to get next token function and instance of user's lexer that is passed to it
 */
struct UserLexerGetNextToken {

    /**
     * the instance is passed to the subroutine to maintain state
     */
    void* instance;

    /**
     * the subroutine to get the next token
     */
    EmbeddedLexerGetNextTokenFn subroutine;

};

/**
 * this function is called by parser on a macro as value ex: var x = #html {}
 * the macro is a value, so a value must be returned from this function
 */
typedef Value*(*EmbeddedParseMacroValueFn)(Parser* parser, ASTAllocator* allocator);

/**
 * this function is called by parser on a macro as node ex: #html {}
 * the macro is a node, so a node must be returned from this function
 */
typedef ASTNode*(*EmbeddedParseMacroNodeFn)(Parser* parser, ASTAllocator* allocator);