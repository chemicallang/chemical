// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/structures/Scope.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/values/CastedValue.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/statements/AliasStmt.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"

AccessSpecifier get_specifier_from(TokenType type) {
    switch(type) {
        case TokenType::PublicKw:
        default:
            return AccessSpecifier::Public;
        case TokenType::InternalKw:
            return AccessSpecifier::Internal;
        case TokenType::PrivateKw:
            return AccessSpecifier::Private;
        case TokenType::ProtectedKw:
            return AccessSpecifier::Protected;
    }
}

ASTNode* Parser::parseTopLevelAccessSpecifiedDecl(ASTAllocator& allocator, AccessSpecifier spec, bool comptime) {
    switch (token->type) {
        case TokenType::FuncKw:
            return (ASTNode*) parseFunctionStructureTokens(allocator, spec, false, true, comptime);
        case TokenType::ComptimeKw:
            if(comptime) {
                error("already inside comptime context");
            }
            token++;
            return (ASTNode*) parseTopLevelAccessSpecifiedDecl(allocator, spec, true);
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
            return parseVarInitializationTokens(allocator, spec, true, false, true, comptime);
        case TokenType::InterfaceKw:
            return (ASTNode*) parseInterfaceStructureTokens(allocator, spec);
        case TokenType::ImplKw:
            return (ASTNode*) parseImplTokens(allocator, spec);
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

ASTNode* Parser::parseTopLevelStatement(ASTAllocator& allocator, bool comptime) {
    const auto tokenType = token->type;
    switch(tokenType) {
        case TokenType::ImportKw:
            return (ASTNode*) parseImportStatement(allocator);
        case TokenType::PublicKw:
            token++;
            return parseTopLevelAccessSpecifiedDecl(global_allocator, AccessSpecifier::Public, comptime);
        case TokenType::ProtectedKw:
            token++;
            return parseTopLevelAccessSpecifiedDecl(global_allocator, AccessSpecifier::Protected, comptime);
        case TokenType::PrivateKw:
            token++;
            return parseTopLevelAccessSpecifiedDecl(allocator, AccessSpecifier::Private, comptime);
        case TokenType::InternalKw:
            token++;
            return parseTopLevelAccessSpecifiedDecl(allocator, AccessSpecifier::Internal, comptime);
        case TokenType::ComptimeKw:
            if(comptime) {
                error("already inside comptime context");
            }
            token++;
            switch(token->type) {
                case TokenType::LBrace:
                    return (ASTNode*) parseComptimeBlockNoKw(allocator);
                case TokenType::IfKw:
                    return (ASTNode*) parseIfStatement(allocator, false, false, false, true);
                default:
                    return parseTopLevelStatement(allocator, true);
            }
        case TokenType::ConstKw:
        case TokenType::VarKw:
            return parseVarInitializationTokens(allocator, AccessSpecifier::Internal, true, false, true, comptime);
        case TokenType::UsingKw:
            return (ASTNode*) parseUsingStatement(allocator);
        case TokenType::HashMacro:
            return parseMacroNode(allocator, CBIFunctionType::ParseMacroTopLevelNode);
        case TokenType::ProvideKw:
            return (ASTNode*) parseProvideStatement(allocator);
        case TokenType::AliasKw:
            error("alias statement not supported as a top level declaration");
            return nullptr;
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
            return (ASTNode*) parseIfStatement(global_allocator, false, false, true, true);
        case TokenType::FuncKw:
            return (ASTNode*) parseFunctionStructureTokens(allocator, AccessSpecifier::Internal, false, true, comptime);
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
            return parseVarInitializationTokens(allocator, AccessSpecifier::Internal, false, true, true, false);
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
            return parseMacroNode(allocator, CBIFunctionType::ParseMacroNode);
        case TokenType::ReturnKw:
            return (ASTNode*) parseReturnStatement(allocator);
        case TokenType::DeallocKw:
            return (ASTNode*) parseDeallocStatement(allocator);
        case TokenType::DestructKw:
        case TokenType::DeleteKw:
            return (ASTNode*) parseDestructStatement(allocator);
        case TokenType::ThrowKw:
            return (ASTNode*) parseThrowStatement(allocator);
        case TokenType::UsingKw:
            return (ASTNode*) parseUsingStatement(allocator);
        case TokenType::ProvideKw:
            return (ASTNode*) parseProvideStatement(allocator);
        case TokenType::NewKw:
            return (ASTNode*) parsePlacementNewNode(allocator);
        case TokenType::ComptimeKw:
            token++;
            switch(token->type) {
                case TokenType::LBrace:
                    return parseComptimeBlockNoKw(allocator);
                case TokenType::IfKw:
                    return (ASTNode*) parseIfStatement(allocator, false, false, false, true);
                default:
                    return parseComptimeBlockNoKw(allocator);
            }
        case TokenType::IfKw:
            return (ASTNode*) parseIfStatement(allocator, is_value, parse_value_node, false, false);
        case TokenType::TryKw:
            return (ASTNode*) parseTryCatch(allocator);
        case TokenType::TypeKw:
            return (ASTNode*) parseTypealiasStatement(allocator, AccessSpecifier::Internal);
        case TokenType::SwitchKw:
            return (ASTNode*) parseSwitchStatementBlock(allocator, is_value, parse_value_node);
        case TokenType::LoopKw:
            return (ASTNode*) parseLoopBlockTokens(allocator);
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

DeallocStmt* Parser::parseDeallocStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::DeallocKw) {
        token++;
        auto stmt = new (allocator.allocate<DeallocStmt>()) DeallocStmt(nullptr, parent_node, loc_single(tok));
        auto expr = parseAccessChainOrAddrOf(allocator);
        if(expr) {
            stmt->ptr = expr;
        } else {
            error("expected a value after 'dealloc'");;
        }
        return stmt;
    } else {
        return nullptr;
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
        return stmt;
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
        auto chain = new (allocator.allocate<AccessChain>()) AccessChain(location);
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

#ifdef LSP_BUILD
        id->linked = alias;
#endif

        if(!consumeToken(TokenType::EqualSym)) {
            error() << "expected an equal symbol after the alias keyword";
            return nullptr;
        }

        const auto value = parseAccessChain(allocator);
        if(!value) {
            error() << "expected a value for alias statement";
            return nullptr;
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
        case TokenType::MultilineString:
            return (Value*) parseStringValue(allocator);
        case TokenType::BacktickString:
        case TokenType::StringExprStart:
            return (Value*) parseExpressiveString(allocator);
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
            return new(allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, typeBuilder.getBoolType(), loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), loc_single(t));
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
        case TokenType::DynKw: {
            token++;
            return parseDynamicValue(allocator);
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

ComptimeBlock* Parser::parseComptimeBlockNoKw(ASTAllocator& allocator) {
    auto ctBlock = new(allocator.allocate<ComptimeBlock>()) ComptimeBlock(parent_node, loc_single(token));
    auto block = parseBraceBlock("comptime", ctBlock, allocator);
    if (block.has_value()) {
        ctBlock->body = std::move(block.value());
    } else {
        error("missing body for comptime block");
    }
    return ctBlock;
}

bool BasicParser::parseModuleDefinition(ASTAllocator& allocator, ModuleFileData& data) {

    if(token->type == TokenType::Identifier && token->value == "module") {
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
        data.module_name = allocate_view(allocator, scope_name->value);
        return true;
    }

    const auto mod_name = consumeIdentifierOrKeyword();
    if(!mod_name) {
        error("expected an identifier for module definition");
        return false;
    }

    data.scope_name = allocate_view(allocator, scope_name->value);
    data.module_name = allocate_view(allocator, mod_name->value);

    return true;

}

ModFileIfBase* parseModFileId(BasicParser& parser, ASTAllocator& allocator) {
    const auto is_neg = parser.token->type == TokenType::NotSym;
    if(is_neg) parser.token++;
    auto id = parser.consumeIdentifierOrKeyword();
    if(id) {
        const auto if_id = allocator.allocate_released<ModFileIfId>();
        if_id->is_id = true;
        if_id->is_negative = is_neg;
        if_id->value = parser.allocate_view(allocator, id->value);
        return if_id;
    } else {
        return nullptr;
    }
}

IffyBase* BasicParser::parseIffyConditional(ASTAllocator& allocator) {

    auto& parser = *this;

    auto first = parseModFileId(parser, allocator);
    if(!first) {
        return nullptr;
    }

    const auto tok_type = token->type;
    const auto isLogicalAnd = tok_type == TokenType::LogicalAndSym;
    if(!(isLogicalAnd || tok_type == TokenType::LogicalOrSym)) {
        return first;
    }

    token++;

    // const auto loc_first = loc_single(token);
    auto second = parseModFileId(parser, allocator);
    if(!second) {
        parser.error("expected a second value in expression");
        return first;
    }

    const auto if_expr = allocator.allocate_released<ModFileIfExpr>();
    if_expr->is_id = false;
    if_expr->left = first;
    if_expr->right = second;
    if_expr->op = isLogicalAnd ? ModFileIfExprOp::And : ModFileIfExprOp::Or;

    auto rootExpr = if_expr;
    auto currentExpr = rootExpr;

    while(true) {

        const auto type = token->type;
        const auto isNextLogicalAnd = type == TokenType::LogicalAndSym;

        if(!(isNextLogicalAnd || type == TokenType::LogicalOrSym)) {
            return rootExpr;
        }

        token++;
        // const auto loc = loc_single(token);
        const auto secondId = parseModFileId(parser, allocator);
        if(!secondId) {
            parser.error("expected second value in expression");
            return rootExpr;
        }

        // A | B & C | D = (A | (B & C)) | D

        if(isNextLogicalAnd && currentExpr->op != ModFileIfExprOp::And) {

            const auto newExpr = allocator.allocate_released<ModFileIfExpr>();
            newExpr->is_id = false;
            newExpr->left = currentExpr->right;
            newExpr->right = secondId;
            newExpr->op = ModFileIfExprOp::And;

            currentExpr->right = newExpr;
        } else {

            const auto newExpr = allocator.allocate_released<ModFileIfExpr>();
            newExpr->is_id = false;
            newExpr->left = currentExpr;
            newExpr->right = secondId;
            newExpr->op = isNextLogicalAnd ? ModFileIfExprOp::And : ModFileIfExprOp::Or;

            currentExpr = newExpr;
            rootExpr = currentExpr;

        }

    }

}

bool BasicParser::parseSourceStmt(ASTAllocator& allocator, ModuleFileData& data) {

    if(token->type == TokenType::Identifier && token->value == "source") {
        token++;
    } else {
        return false;
    }

    data.sources_list.emplace_back();
    auto& source = data.sources_list.back();

    // get the source path
    auto sourcePath = parseString(allocator);
    if(sourcePath.has_value()) {
        source.path = sourcePath.value();
    } else {
        error("expected a source path");
        return false;
    }

    if(consumeToken(TokenType::IfKw)) {
        const auto cond = parseIffyConditional(allocator);
        source.if_cond = cond;
        if(!cond) {
            error("expected condition after 'if' in source statement");
        }
    }

    return true;

}

bool BasicParser::parseLinkStmt(ASTAllocator& allocator, ModuleFileData& data) {

    if(token->type == TokenType::Identifier && token->value == "link") {
        token++;
    } else {
        return false;
    }

    data.link_libs.emplace_back();
    auto& link_lib = data.link_libs.back();

    // get the library name
    auto libName = parseString(allocator);
    if(libName.has_value()) {
        link_lib.name = libName.value();
    } else {
        error("expected a library name");
        return false;
    }

    // condition is required
    if(consumeToken(TokenType::IfKw)) {
        const auto cond = parseIffyConditional(allocator);
        link_lib.if_cond = cond;
        if(!cond) {
            error("expected condition after 'if' in link statement");
        }
    }

    return true;

}