// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/frontend/AnnotationController.h"
#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/UsingStmt.h"
#include "compiler/cbi/model/ASTBuilder.h"

bool Parser::parseAnnotation(ASTAllocator& allocator) {
    if(token->type != TokenType::Annotation) {
        return false;
    }
    const auto annot = token;
    token++;
    auto name_view = chem::string_view(annot->value.data() + 1, annot->value.size() - 1);
    auto definition = controller.get_definition(name_view);
    if(definition == nullptr) {
        error() << "unknown annotation found '" << annot->value << "'";
        return true;
    }
    annotations.emplace_back(*definition, std::vector<Value*>{});
    if(!consumeToken(TokenType::LParen)) {
        return true;
    }
    auto& saved_annot = annotations.back();
    auto& args = saved_annot.arguments;
    do {
        consumeNewLines();
        auto expr = parseExpressionOrArrayOrStruct(allocator);
        if(expr) {
            args.emplace_back(expr);
        } else {
            break;
        }
    } while (consumeToken(TokenType::CommaSym));
    consumeNewLines();
    if (!consumeToken(TokenType::RParen)) {
        unexpected_error("expected a ')' for a function call, after starting '('");
        return true;
    }
    return true;
}

void Parser::annotate(ASTNode* node) {
    for(auto& annot : annotations) {
        controller.handle_annotation(annot.definition, this, node, annot.arguments);
    }
    annotations.clear();
}

Value* Parser::parseMacroValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
        auto found = binder->findHook(view, CBIFunctionType::ParseMacroValue);
        if(found) {
            token++;
            ASTBuilder builder(&allocator, typeBuilder);
            const auto parsedValue = (EmbeddedParseMacroValueFn (found))(this, &builder);
#ifdef LSP_BUILD
            t.linked = parsedValue;
#endif
            return parsedValue;
        } else {
            error() << "couldn't find macro parser for '" << t.value << "'";
        }
    }
    return nullptr;
}

ASTNode* Parser::parseMacroNodeTopLevel(ASTAllocator& allocator, AccessSpecifier spec, CBIFunctionType type) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
        auto found = binder->findHook(view, type);
        if(found) {
            token++;
            ASTBuilder builder(&allocator, typeBuilder);
            const auto parsedNode = (EmbeddedParseMacroNodeTopLevelFn (found))(this, &builder, (int) spec);
#ifdef LSP_BUILD
            t.linked = parsedNode;
#endif
            return parsedNode;
        } else {
            error() << "couldn't find macro parser for '" << t.value << "'";
        }
    }
    return nullptr;
}

ASTNode* Parser::parseMacroNode(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
        auto found = binder->findHook(view, CBIFunctionType::ParseMacroNode);
        if(found) {
            token++;
            ASTBuilder builder(&allocator, typeBuilder);
            const auto parsedNode = (EmbeddedParseMacroNodeFn (found))(this, &builder);
#ifdef LSP_BUILD
            t.linked = parsedNode;
#endif
            return parsedNode;
        } else {
            error() << "couldn't find macro parser for '" << t.value << "'";
        }
    }
    return nullptr;
}