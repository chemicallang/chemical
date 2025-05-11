// Copyright (c) Chemical Language Foundation 2025.

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
#include "ast/statements/AliasStmt.h"
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
            return (ASTNode*) parseFunctionStructureTokens(allocator, spec, false, true);
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
        case TokenType::TypeKw:
            return (ASTNode*) parseTypealiasStatement(allocator, spec);
        case TokenType::NamespaceKw:
            return (ASTNode*) parseNamespace(allocator, spec);
        case TokenType::AliasKw:
            return (ASTNode*) parseAliasStatement(allocator, spec);
        default:
            error("unknown token, expected a top level declaration");
            return nullptr;
    }
}

ASTNode* Parser::parseTopLevelStatement(ASTAllocator& allocator) {
    // TODO remove this
    while(true) {
        const auto type = token->type;
        if(type == TokenType::NewLine) {
            token++;
        } else {
            break;
        }
    }
    switch(token->type) {
        case TokenType::PublicKw:
        case TokenType::PrivateKw:
        case TokenType::InternalKw:
            return parseTopLevelAccessSpecifiedDecls(allocator);
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
        case TokenType::AliasKw:
            return (ASTNode*) parseAliasStatement(allocator, AccessSpecifier::Internal);
        case TokenType::EnumKw:
            return (ASTNode*) parseEnumStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::StructKw:
            return (ASTNode*) parseStructStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::UnionKw:
            return (ASTNode*) parseUnionStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::VariantKw:
            return (ASTNode*) parseVariantStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::TypeKw:
            return (ASTNode*) parseTypealiasStatement(allocator, AccessSpecifier::Internal);
        case TokenType::InterfaceKw:
            return (ASTNode*) parseInterfaceStructureTokens(allocator, AccessSpecifier::Internal);
        case TokenType::ImplKw:
            return (ASTNode*) parseImplTokens(allocator, AccessSpecifier::Internal);
        case TokenType::IfKw:
            // top level if can contain public declarations, which is why it needs to be allocated
            // on global allocator, so other modules can import nodes inside the if
            return (ASTNode*) parseIfStatement(global_allocator, false, false, true);
        case TokenType::FuncKw:
            return (ASTNode*) parseFunctionStructureTokens(allocator, AccessSpecifier::Internal, false, true);
        case TokenType::NamespaceKw:
            return (ASTNode*) parseNamespace(allocator, AccessSpecifier::Internal);
        default:
            return nullptr;
    }
}

ASTNode* Parser::parseNestedLevelStatementTokens(ASTAllocator& allocator, bool is_value, bool parse_value_node) {
    // TODO remove this
    while(true) {
        const auto type = token->type;
        if(type == TokenType::NewLine) {
            token++;
        } else {
            break;
        }
    }
    switch(token->type) {
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
        case TokenType::AliasKw:
            return (ASTNode*) parseAliasStatement(allocator, AccessSpecifier::Internal);
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
        case TokenType::TypeKw:
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
        auto has_namespace = consumeToken(TokenType::NamespaceKw);
        const auto location = loc_single(tok);
        auto chain = new (allocator.allocate<AccessChain>()) AccessChain(false, location);
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

AliasStmt* Parser::parseAliasStatement(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeToken(TokenType::AliasKw)) {
        const auto id = consumeIdentifierOrKeyword();
        if(!id) {
            error() << "expected an identifier after the alias keyword";
            return nullptr;
        }

        const auto alias = new (allocator.allocate<AliasStmt>()) AliasStmt(specifier, allocate_view(allocator, id->value), nullptr, parent_node, loc_single(id));

        if(!consumeToken(TokenType::EqualSym)) {
            error() << "expected an equal symbol after the alias keyword";
            return alias;
        }

        const auto value = parseAccessChain(allocator);
        if(!value) {
            error() << "expected a value for alias statement";
            return alias;
        }

        alias->value = value;

        return alias;
    }
    return nullptr;
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
        case TokenType::UnsafeKw: {
            token++;
            return parseUnsafeValue(allocator);
        }
        case TokenType::ComptimeKw: {
            token++;
            return parseComptimeValue(allocator);
        }
        case TokenType::SizeOfKw: {
            token++;
            return parseSizeOfValue(allocator);
        }
        case TokenType::AlignOfKw: {
            token++;
            return parseAlignOfValue(allocator);
        }
        default:
            return (Value*) parseAccessChain(allocator, false);
    }
}

ProvideStmt* Parser::parseProvideStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ProvideKw) {
        token++;
        auto stmt = new (allocator.allocate<ProvideStmt>()) ProvideStmt(nullptr, "", parent_node, loc_single(tok));
        auto val = parseProvideValue(allocator);
        if(val) {
            stmt->value = val;
        } else {
            error("expected a value after provide keyword");
            return nullptr;
        }
        if(!consumeToken(TokenType::AsKw)) {
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
        auto ctBlock = new (allocator.allocate<ComptimeBlock>()) ComptimeBlock(parent_node, loc_single(tok));
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

bool Parser::skipModuleDefinition(ASTAllocator& allocator) {

    if(token->value == "module") {
        token++;
    } else {
        error("expected a module definition at top");
        return false;
    }

    const auto scope_name = consumeIdentifierOrKeyword();
    if(!scope_name) {
        error("expected an identifier for module definition");
        return false;
    }

    const auto t = token->type;
    if(t == TokenType::DotSym || t == TokenType::DoubleColonSym) {
        token++;
    } else {
        return true;
    }

    const auto mod_name = consumeIdentifierOrKeyword();
    if(!mod_name) {
        error("expected an identifier for module definition");
        return false;
    }

    return true;

}