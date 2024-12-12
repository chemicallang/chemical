// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/structures/Scope.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/values/CastedValue.h"
#include "ast/statements/ThrowStatement.h"

ASTNode* Parser::parseTopLevelAccessSpecifiedDecls(ASTAllocator& local_allocator) {
    auto specifier = parseAccessSpecifier();
    if(!specifier.has_value()) {
        return nullptr;
    }
    readWhitespace();
    auto spec = specifier.value();
    auto& allocator = spec == AccessSpecifier::Public ? global_allocator : local_allocator;
    auto decl = parseFunctionStructureTokens(allocator, spec, true, true);
    if(decl) {
        return (ASTNode*) decl;
    }
    auto enumDecl = parseEnumStructureTokens(allocator, spec);
    if(enumDecl) {
        return (ASTNode*) enumDecl;
    }
    auto structDecl = parseStructStructureTokens(allocator, spec);
    if(structDecl) {
        return (ASTNode*) structDecl;
    }
    auto unionDecl = parseUnionStructureTokens(allocator, spec);
    if(unionDecl) {
        return (ASTNode*) unionDecl;
    }
    auto variantDecl = parseVariantStructureTokens(allocator, spec);
    if(variantDecl) {
        return (ASTNode*) variantDecl;
    }
    auto varInit = parseVarInitializationTokens(allocator, spec);
    if(varInit) {
        return (ASTNode*) varInit;
    }
    auto interface = parseInterfaceStructureTokens(allocator, spec);
    if(interface) {
        return (ASTNode*) interface;
    }
    auto alias = parseTypealiasStatement(allocator, spec);
    if(alias) {
        return (ASTNode*) alias;
    }
    auto ns = parseNamespace(allocator, spec);
    if(ns) {
        return (ASTNode*) ns;
    }
    error("expected a top level declaration after access specifier");
    return nullptr;
}

ASTNode* Parser::parseTopLevelStatement(ASTAllocator& allocator) {
    auto node = parseTopLevelAccessSpecifiedDecls(allocator);
    if(node) {
        return node;
    }
    auto comment = parseSingleLineComment(allocator);
    if(comment) {
        return (ASTNode*) comment;
    }
    auto multiline = parseMultiLineComment(allocator);
    if(multiline) {
        return (ASTNode*) multiline;
    }
    auto init = parseVarInitializationTokens(allocator, AccessSpecifier::Internal, true);
    if(init) {
        return (ASTNode*) init;
    }
    auto usingStmt = parseUsingStatement(allocator);
    if(usingStmt) {
        return (ASTNode*) usingStmt;
    }
    auto provideStmt = parseProvideStatement(allocator);
    if(provideStmt) {
        return (ASTNode*) provideStmt;
    }
    auto comptimeBlock = parseComptimeBlock(allocator);
    if(comptimeBlock) {
        return (ASTNode*) comptimeBlock;
    }
    auto enumDecl = parseEnumStructureTokens(allocator, AccessSpecifier::Internal);
    if(enumDecl) {
        return (ASTNode*) enumDecl;
    }
    auto structDecl = parseStructStructureTokens(allocator, AccessSpecifier::Internal);
    if(structDecl) {
        return (ASTNode*) structDecl;
    }
    auto unionDecl = parseUnionStructureTokens(allocator, AccessSpecifier::Internal);
    if(unionDecl) {
        return (ASTNode*) unionDecl;
    }
    auto variantDecl = parseVariantStructureTokens(allocator, AccessSpecifier::Internal);
    if(variantDecl) {
        return (ASTNode*) variantDecl;
    }
    auto alias = parseTypealiasStatement(allocator, AccessSpecifier::Internal);
    if(alias) {
        return (ASTNode*) alias;
    }
    auto interface = parseInterfaceStructureTokens(allocator, AccessSpecifier::Internal);
    if(interface) {
        return (ASTNode*) interface;
    }
    auto impl = parseImplTokens(allocator, AccessSpecifier::Internal);
    if(impl) {
        return (ASTNode*) impl;
    }
    auto ifStmt = parseIfStatement(allocator, false, false, true);
    if(ifStmt) {
        return (ASTNode*) ifStmt;
    }
    auto funcDecl = parseFunctionStructureTokens(allocator, AccessSpecifier::Internal, true, true);
    if(funcDecl) {
        return (ASTNode*) funcDecl;
    }
    auto ns = parseNamespace(allocator, AccessSpecifier::Internal);
    if(ns) {
        return (ASTNode*) ns;
    }
    return nullptr;
}

ASTNode* Parser::parseNestedLevelStatementTokens(ASTAllocator& allocator, bool is_value, bool parse_value_node) {
    auto comment = parseSingleLineComment(allocator);
    if(comment) {
        return (ASTNode*) comment;
    }
    auto multiline = parseMultiLineComment(allocator);
    if(multiline) {
        return (ASTNode*) multiline;
    }
    auto init = parseVarInitializationTokens(allocator, AccessSpecifier::Internal);
    if(init) {
        return (ASTNode*) init;
    }
    auto importStmt = parseImportStatement(allocator);
    if(importStmt) {
        return (ASTNode*) importStmt;
    }
    auto initBlock = parseConstructorInitBlock(allocator);
    if(initBlock) {
        return (ASTNode*) initBlock;
    }
    auto unsafeBlock = parseUnsafeBlock(allocator);
    if(unsafeBlock) {
        return (ASTNode*) unsafeBlock;
    }
    auto breakStmt = parseBreakStatement(allocator);
    if(breakStmt) {
        return (ASTNode*) breakStmt;
    }
    auto continueStmt = parseContinueStatement(allocator);
    if(continueStmt) {
        return (ASTNode*) continueStmt;
    }
    auto unrStmt = parseUnreachableStatement(allocator);
    if(unrStmt) {
        return (ASTNode*) unrStmt;
    }
    auto returnStmt = parseReturnStatement(allocator);
    if(returnStmt) {
        return (ASTNode*) returnStmt;
    }
    auto destructStmt = parseDestructStatement(allocator);
    if(destructStmt) {
        return (ASTNode*) destructStmt;
    }
    auto throwStmt = parseThrowStatement(allocator);
    if(throwStmt) {
        return (ASTNode*) throwStmt;
    }
    auto usingStmt = parseUsingStatement(allocator);
    if(usingStmt) {
        return (ASTNode*) usingStmt;
    }
    auto provideStmt = parseProvideStatement(allocator);
    if(provideStmt) {
        return (ASTNode*) provideStmt;
    }
    auto comptimeBlock = parseComptimeBlock(allocator);
    if(comptimeBlock) {
        return (ASTNode*) comptimeBlock;
    }
    auto ifStmt = parseIfStatement(allocator, is_value, parse_value_node, false);
    if(ifStmt) {
        return (ASTNode*) ifStmt;
    }
    auto tryCatch = parseTryCatch(allocator);
    if(tryCatch) {
        return (ASTNode*) tryCatch;
    }
    auto alias = parseTypealiasStatement(allocator, AccessSpecifier::Internal);
    if(alias) {
        return (ASTNode*) alias;
    }
    auto switchStmt = parseSwitchStatementBlock(allocator, is_value, parse_value_node);
    if(switchStmt) {
        return (ASTNode*) switchStmt;
    }
    auto loopBlock = parseLoopBlockTokens(allocator, is_value);
    if(loopBlock) {
        return (ASTNode*) loopBlock;
    }
    auto forLoop = parseForLoop(allocator);
    if(forLoop) {
        return (ASTNode*) forLoop;
    }
    auto doWhile = parseDoWhileLoop(allocator);
    if(doWhile) {
        return (ASTNode*) doWhile;
    }
    auto whileLoop = parseWhileLoop(allocator);
    if(whileLoop) {
        return (ASTNode*) whileLoop;
    }
    auto assignment = parseAssignmentStmt(allocator);
    if(assignment) {
        return assignment;
    }
}

ThrowStatement* Parser::parseThrowStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ThrowKw) {
        token++;
        auto stmt = new (allocator.allocate<ThrowStatement>()) ThrowStatement(nullptr, parent_node, loc_single(tok));
        readWhitespace();
        auto expr = parseExpression(allocator);
        if(expr) {
            stmt->value = expr;
        } else {
            error("expected a value after 'throw'");;
        }
    } else {
        return nullptr;
    }
}

UsingStmt* Parser::parseUsingStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::UsingKw) {
        token++;
        readWhitespace();
        auto has_namespace = consumeWSOfType(TokenType::NamespaceKw);
        auto stmt = new (allocator.allocate<UsingStmt>()) UsingStmt({}, parent_node, has_namespace, loc_single(tok));
        do {
            auto id = parseVariableIdentifier(allocator);
            if(id) {
                stmt->chain.values.emplace_back((ChainValue*) id);
            } else {
                error("expected a identifier in using statement");
                return stmt;
            };
        } while(consumeToken(TokenType::DoubleColonSym));
        return stmt;
    } else {
        return nullptr;
    }
}

Value* Parser::parseProvideValue(ASTAllocator& allocator) {
    auto acValue = parseAccessChainValueToken(allocator);
    if(acValue) {
        return acValue;
    }
    auto notValue = parseNotValue(allocator);
    if(notValue) {
        return (Value*) notValue;
    }
    auto negValue = parseNegativeValue(allocator);
    if(negValue) {
        return (Value*) negValue;
    }
    auto ac = parseAccessChainOrAddrOf(allocator, false);
    if(ac) {
        return ac;
    }
    auto macroVal = parseMacroValue(allocator);
    if(macroVal) {
        return macroVal;
    }
    return nullptr;
}

ProvideStmt* Parser::parseProvideStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ProvideKw) {
        token++;
        readWhitespace();
        auto stmt = new (allocator.allocate<ProvideStmt>()) ProvideStmt(nullptr, "", { nullptr, 0 }, parent_node, loc_single(tok));
        auto val = parseProvideValue(allocator);
        if(val) {
            stmt->value = val;
        } else {
            error("expected a value after provide keyword");
            return nullptr;
        }
        readWhitespace();
        auto asKw = consumeWSOfType(TokenType::AsKw);
        if(!asKw) {
            error("expected 'as' keyword after provide (expression)");
            return stmt;
        }
        readWhitespace();
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            stmt->identifier = id->value;
        } else {
            error("expected identifier after 'as'");
            return stmt;
        }
        auto block = parseBraceBlock("provide", stmt, allocator);
        if(block.has_value()) {
            stmt->body = std::move(block.value());
        } else {
            error("missing body for provide statement");
            return stmt;
        }
        return stmt;
    } else {
        return nullptr;
    }
}

ComptimeBlock* Parser::parseComptimeBlock(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ComptimeKw) {
        token++;
        readWhitespace();
        auto ctBlock = new (allocator.allocate<ComptimeBlock>()) ComptimeBlock({ nullptr, 0 }, parent_node, loc_single(tok));
        auto block = parseBraceBlock("comptime", ctBlock, allocator);
        if(block.has_value()) {
            ctBlock->body = std::move(block.value());
        } else {
            error("missing body for provide statement");
        }
        return ctBlock;
    } else {
        return nullptr;
    }
}