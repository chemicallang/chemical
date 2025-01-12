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
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"

ASTNode* Parser::parseTopLevelAccessSpecifiedDecls(ASTAllocator& local_allocator) {
    auto specifier = parseAccessSpecifier();
    if(!specifier.has_value()) {
        return nullptr;
    }
    auto spec = specifier.value();
    auto& allocator = spec == AccessSpecifier::Public ? global_allocator : local_allocator;
    switch (token->type) {
        case TokenType::FuncKw:
            return (ASTNode*) parseFunctionStructureTokens(allocator, spec, true, true);
        case TokenType::EnumKw:
            return (ASTNode*) parseEnumStructureTokens(allocator, spec);
        case TokenType::StructKw:
            return (ASTNode*) parseStructStructureTokens(allocator, spec);
        case TokenType::UnionKw:
            return (ASTNode*) parseUnionStructureTokens(allocator, spec);
        case TokenType::VariantKw:
            return (ASTNode*) parseVariantStructureTokens(allocator, spec);
        case TokenType::VarKw:
        case TokenType::ConstKw:
            return (ASTNode*) parseVarInitializationTokens(allocator, spec);
        case TokenType::InterfaceKw:
            return (ASTNode*) parseInterfaceStructureTokens(allocator, spec);
        case TokenType::TypealiasKw:
            return (ASTNode*) parseTypealiasStatement(allocator, spec);
        case TokenType::NamespaceKw:
            return (ASTNode*) parseNamespace(allocator, spec);
        default:
            error("unknown token, expected a top level declaration");
            return nullptr;
    }
}

ASTNode* Parser::parseTopLevelStatement(ASTAllocator& allocator) {
    switch(token->type) {
        case TokenType::PublicKw:
        case TokenType::PrivateKw:
        case TokenType::InternalKw:
            return parseTopLevelAccessSpecifiedDecls(allocator);
        case TokenType::SingleLineComment:
            return (ASTNode*) parseSingleLineComment(allocator);
        case TokenType::MultiLineComment:
            return (ASTNode*) parseMultiLineComment(allocator);
        case TokenType::ConstKw:
        case TokenType::VarKw:
            return (ASTNode*) parseVarInitializationTokens(allocator, AccessSpecifier::Internal, true);
        case TokenType::UsingKw:
            return (ASTNode*) parseUsingStatement(allocator);
        case TokenType::HashMacro:
            return parseMacroNode(allocator);
        case TokenType::ProvideKw:
            return (ASTNode*) parseProvideStatement(allocator);
        case TokenType::ComptimeKw:
            return (ASTNode*) parseComptimeBlock(allocator);
        case TokenType::EnumKw:
            return (ASTNode*) parseEnumStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::StructKw:
            return (ASTNode*) parseStructStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::UnionKw:
            return (ASTNode*) parseUnionStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::VariantKw:
            return (ASTNode*) parseVariantStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::TypealiasKw:
            return (ASTNode*) parseTypealiasStatement(allocator, AccessSpecifier::Internal);
        case TokenType::InterfaceKw:
            return (ASTNode*) parseInterfaceStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::ImplKw:
            return (ASTNode*) parseImplTokens(allocator, AccessSpecifier::Internal);
        case TokenType::IfKw:
            return (ASTNode*) parseIfStatement(allocator, false, false, true);
        case TokenType::FuncKw:
            return (ASTNode*) parseFunctionStructureTokens(allocator, AccessSpecifier::Internal, true, true);
        case TokenType::NamespaceKw:
            return (ASTNode*) parseNamespace(allocator, AccessSpecifier::Internal);
        default:
            return nullptr;
    }
}

ASTNode* Parser::parseNestedLevelStatementTokens(ASTAllocator& allocator, bool is_value, bool parse_value_node) {
    switch(token->type) {
        case TokenType::SingleLineComment:
            return (ASTNode*) parseSingleLineComment(allocator);
        case TokenType::MultiLineComment:
            return (ASTNode*) parseMultiLineComment(allocator);
        case TokenType::VarKw:
        case TokenType::ConstKw:
            return (ASTNode*) parseVarInitializationTokens(allocator, AccessSpecifier::Internal);
        case TokenType::ImportKw:
            return (ASTNode*) parseImportStatement(allocator);
        case TokenType::InitKw:
            return (ASTNode*) parseConstructorInitBlock(allocator);
        case TokenType::UnsafeKw:
            return (ASTNode*) parseUnsafeBlock(allocator);
        case TokenType::BreakKw:
            return (ASTNode*) parseBreakStatement(allocator);
        case TokenType::ContinueKw:
            return (ASTNode*) parseContinueStatement(allocator);
        case TokenType::UnreachableKw:
            return (ASTNode*) parseUnreachableStatement(allocator);
        case TokenType::HashMacro:
            return parseMacroNode(allocator);
        case TokenType::ReturnKw:
            return (ASTNode*) parseReturnStatement(allocator);
        case TokenType::DestructKw:
            return (ASTNode*) parseDestructStatement(allocator);
        case TokenType::ThrowKw:
            return (ASTNode*) parseThrowStatement(allocator);
        case TokenType::UsingKw:
            return (ASTNode*) parseUsingStatement(allocator);
        case TokenType::ProvideKw:
            return (ASTNode*) parseProvideStatement(allocator);
        case TokenType::NewKw:
            return (ASTNode*) parseNewValueAsNode(allocator);
        case TokenType::ComptimeKw:
            return (ASTNode*) parseComptimeBlock(allocator);
        case TokenType::IfKw:
            return (ASTNode*) parseIfStatement(allocator, is_value, parse_value_node, false);
        case TokenType::TryKw:
            return (ASTNode*) parseTryCatch(allocator);
        case TokenType::TypealiasKw:
            return (ASTNode*) parseTypealiasStatement(allocator, AccessSpecifier::Internal);
        case TokenType::SwitchKw:
            return (ASTNode*) parseSwitchStatementBlock(allocator, is_value, parse_value_node);
        case TokenType::LoopKw:
            return (ASTNode*) parseLoopBlockTokens(allocator, is_value);
        case TokenType::ForKw:
            return (ASTNode*) parseForLoop(allocator);
        case TokenType::DoKw:
            return (ASTNode*) parseDoWhileLoop(allocator);
        case TokenType::WhileKw:
            return (ASTNode*) parseWhileLoop(allocator);
        default:
            return parseAssignmentStmt(allocator);

    }
}

ThrowStatement* Parser::parseThrowStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ThrowKw) {
        token++;
        auto stmt = new (allocator.allocate<ThrowStatement>()) ThrowStatement(nullptr, parent_node, loc_single(tok));
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
        auto has_namespace = consumeWSOfType(TokenType::NamespaceKw);
        const auto location = loc_single(tok);
        auto chain = new (allocator.allocate<AccessChain>()) AccessChain({}, false, location);
        auto stmt = new (allocator.allocate<UsingStmt>()) UsingStmt(chain, parent_node, has_namespace, location);
        do {
            auto id = parseVariableIdentifier(allocator);
            if(id) {
                chain->values.emplace_back((ChainValue*) id);
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
    switch(token->type) {
        case TokenType::Char:
            return parseCharValue(allocator);
        case TokenType::String:
            return parseStringValue(allocator);
        case TokenType::LBracket:
            return (Value*) parseLambdaValue(allocator);
        case TokenType::Number:
            return parseNumberValue(allocator);
        case TokenType::NotSym:
            return (Value*) parseNotValue(allocator);
        case TokenType::MinusSym:
            return (Value*) parseNegativeValue(allocator);
        case TokenType::HashMacro:
            return (Value*) parseMacroValue(allocator);
        case TokenType::AmpersandSym:
            return (Value*) parseAddrOfValue(allocator);
        case TokenType::MultiplySym:
            return (Value*) parseDereferenceValue(allocator);
        case TokenType::NullKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<NullValue>()) NullValue(loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, loc_single(t));
        }
        default:
            return (Value*) parseAccessChain(allocator, false);
    }
}

ProvideStmt* Parser::parseProvideStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ProvideKw) {
        token++;
        auto stmt = new (allocator.allocate<ProvideStmt>()) ProvideStmt(nullptr, "", { nullptr, 0 }, parent_node, loc_single(tok));
        auto val = parseProvideValue(allocator);
        if(val) {
            stmt->value = val;
        } else {
            error("expected a value after provide keyword");
            return nullptr;
        }
        auto asKw = consumeWSOfType(TokenType::AsKw);
        if(!asKw) {
            error("expected 'as' keyword after provide (expression)");
            return stmt;
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            stmt->identifier = allocate_view(allocator, id->value);
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