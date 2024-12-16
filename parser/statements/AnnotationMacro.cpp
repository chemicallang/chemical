// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "ast/base/AnnotableNode.h"
#include "parser/model/CompilerBinder.h"
#include "parser/Parser.h"

const std::unordered_map<chem::string_view, const AnnotationModifierFunc> AnnotationModifierFunctions = {
        { "inline:", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation(AnnotationKind::Inline); } },
        { "inline:always", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation(AnnotationKind::AlwaysInline); } },
        { "noinline", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation(AnnotationKind::NoInline); } },
        { "inline:no", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation(AnnotationKind::NoInline); } },
        { "inline:hint", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation(AnnotationKind::InlineHint); } },
        { "compiler.inline", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::CompilerInline); } },
        { "size:opt", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::OptSize); } },
        { "size:min", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::MinSize); } },
        { "comptime", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::CompTime); } },
        { "compiler.interface", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::CompilerInterface); } },
        { "constructor", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Constructor); } },
        { "make", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Constructor); } },
        { "delete", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Delete); } },
        { "override", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Override); } },
        { "unsafe", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Unsafe); } },
        { "no_init", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::NoInit); }},
        { "extern", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Extern); }},
        { "implicit", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Implicit); }},
        { "propagate", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Propagate); }},
        { "direct_init", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::DirectInit); }},
        { "no_return", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::NoReturn); }},
        { "cpp", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Cpp); }},
        { "clear", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Clear); }},
        { "copy", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Copy); }},
        { "deprecated", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Deprecated); }},
        { "move", [](Parser* parser, AnnotableNode* node) -> void { node->add_annotation( AnnotationKind::Move); }},
};

bool find_annot(Parser* parser, chem::string_view& view) {
    auto found = AnnotationModifierFunctions.find(view);
    if(found != AnnotationModifierFunctions.end()) {
        parser->annotations.emplace_back(found->second);
        return true;
    } else {
        parser->error("unknown annotation found @'" + view.str() + "'");
        return true;
    }
}

bool Parser::parseAnnotation(ASTAllocator& allocator) {
    if(token->type != TokenType::AtSym) {
        return false;
    }
    token++;
    auto tok = consumeIdentifierOrKeyword();
    if(tok) {
        auto next_token_type = token->type;
        if(next_token_type == TokenType::DotSym || next_token_type == TokenType::ColonSym || next_token_type == TokenType::DoubleColonSym) {
            std::string value;
            value.append(tok->value.view());
            while(true) {
                auto& cur_tok = *token;
                auto tok_type = cur_tok.type;
                switch(tok_type) {
                    case TokenType::DotSym:
                        value.append(1, '.');
                        break;
                    case TokenType::DoubleColonSym:
                        value.append(2, ':');
                        break;
                    case TokenType::ColonSym:
                        value.append(1, ':');
                        break;
                    default:
                        if(Token::isKeyword(tok_type) || tok_type == TokenType::Identifier) {
                            value.append(cur_tok.value.view());
                        } else {
                            goto end_loop;
                        }
                }
                token++;
                continue;
                end_loop:
                    break;
            }
            auto view = chem::string_view(value.data(), value.size());
            return find_annot(this, view);
        } else {
            return find_annot(this, tok->value);
        }
    } else {
        error("expected an identifier or keyword after '@' for annotation");
        return true;
    }
}
Value* Parser::parseMacroValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        auto& map = binder->parseMacroValueFunctions;
        const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
        auto found = map.find(view);
        if(found != map.end()) {
            token++;
            return found->second(this, &allocator);
        } else {
            error("couldn't find macro parser for '" + t.value.str() + "'");
        }
    }
    return nullptr;
}