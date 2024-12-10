// Copyright (c) Qinetik 2024.

#pragma once

class Lexer;

class Token;

class Parser;

class Value;

class ASTAllocator;

/**
 * the function that is called to initialize lexer for the user lexer
 */
typedef void(*UserLexerInitializeFn)(Lexer* lexer);

/**
 * the function that is called to provide the next token, if user lexer is active
 */
typedef void(*UserLexerGetNextTokenFn)(Token* returning_token, Lexer* lexer);

/**
 * this function is called when a macro is detected by the parser
 * the macro is a value, so a value must be returned from this function
 */
typedef Value*(*UserParserParseMacroValueFn)(Parser* parser, ASTAllocator* allocator);