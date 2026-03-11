// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class Lexer;

struct Token;

class Parser;

class ASTAny;

class Value;

class ASTNode;

class ASTAllocator;

class ASTBuilder;

class EmbeddedNode;

class EmbeddedValue;

class SymbolResolver;

class SymResLinkBody;

class TopLevelLinkSignature;

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
typedef Value*(*EmbeddedParseMacroValueFn)(Parser* parser, ASTBuilder* builder);

/**
 * this function is called by parser on a macro as node ex: #html {}
 * the macro is a node, so a node must be returned from this function
 */
typedef ASTNode*(*EmbeddedParseMacroNodeFn)(Parser* parser, ASTBuilder* builder);

/**
 * this function is called by parser on a macro as node ex: #html {}
 * the macro is a node, so a node must be returned from this function
 */
typedef ASTNode*(*EmbeddedParseMacroNodeTopLevelFn)(Parser* parser, ASTBuilder* builder, int spec);

/**
 * during link signature this method is called to link the signature of an embedded node
 */
typedef void(*EmbeddedNodeSymResDeclareTopLevel)(SymbolResolver* resolver, EmbeddedNode* node);

/**
 * during link signature this method is called to link the signature of an embedded node
 */
typedef void(*EmbeddedNodeSymResLinkSignature)(TopLevelLinkSignature* visitor, EmbeddedNode* node);

/**
 * during link signature this method is called to link the signature of an embedded value
 */
typedef void(*EmbeddedValueSymResLinkSignature)(TopLevelLinkSignature* visitor, EmbeddedValue* value);

/**
 * symbol resolve function is called to provide linking with different nodes during symbol resolution
 * for example html macro uses it to find methods to call during replacement
 */
typedef void(*EmbeddedNodeSymbolResolveFunc)(SymResLinkBody* visitor, EmbeddedNode* value);

/**
 * symbol resolve function is called to provide linking with different nodes during symbol resolution
 * for example html macro uses it to find methods to call during replacement
 */
typedef bool(*EmbeddedValueSymbolResolveFunc)(SymResLinkBody* visitor, EmbeddedValue* value);

/**
 * replacement function is called to provide ast node that would finally generate code for this
 * given embedded node
 */
typedef ASTNode*(*EmbeddedNodeReplacementFunc)(ASTBuilder* builder, EmbeddedNode* value);

/**
 * replacement function is called to provide ast node that would finally generate code for this
 * given embedded node
 */
typedef Value*(*EmbeddedValueReplacementFunc)(ASTBuilder* builder, EmbeddedValue* value);