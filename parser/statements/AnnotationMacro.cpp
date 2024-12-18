// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "ast/base/AnnotableNode.h"
#include "parser/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/UsingStmt.h"

const std::unordered_map<chem::string_view, const AnnotationModifierFunc> AnnotationModifierFunctions = {
        { "inline", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.is_inline = true;
            } else {
                parser->error("couldn't make the function inline");
            }
        } },
        { "inline:always", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.always_inline = true;
            } else {
                parser->error("couldn't make the function inline always");
            }
        } },
        { "noinline", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.no_inline = true;
            } else {
                parser->error("couldn't make the function noinline");
            }
        } },
        { "inline:no", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.no_inline = true;
            } else {
                parser->error("couldn't make the function noinline");
            }
        } },
        { "inline:hint", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.inline_hint = true;
            } else {
                parser->error("couldn't make the function inline hint");
            }
        } },
        { "compiler.inline", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.compiler_inline = true;
            } else {
                parser->error("couldn't make the function compiler inline");
            }
        } },
        { "size:opt", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.opt_size = true;
            } else {
                parser->error("couldn't make the function opt size");
            }
        } },
        { "size:min", [](Parser* parser, AnnotableNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.min_size = true;
            } else {
                parser->error("couldn't make the function min size");
            }
        } },
        { "comptime", [](Parser* parser, AnnotableNode* node) -> void {
            if(!node->set_comptime(true)) {
                parser->error("couldn't make the declaration comptime");
            }
        } },
        { "compiler.interface", [](Parser* parser, AnnotableNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_compiler_interface(true);
            } else {
                parser->error("couldn't make struct a compiler interface");
            }
        } },
        { "constructor", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_constructor_fn(true);
            } else {
                parser->error("couldn't make the function constructor");
            }
        } },
        { "make", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_constructor_fn(true);
            } else {
                parser->error("couldn't make the function constructor");
            }
        } },
        { "delete", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_delete_fn(true);
            } else {
                parser->error("couldn't make the function a delete function");
            }
        } },
        { "override", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_override(true);
            } else {
                parser->error("couldn't make the function override");
            }
        } },
        { "unsafe", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_unsafe(true);
            } else {
                parser->error("couldn't make the function unsafe");
            }
        } },
        { "no_init", [](Parser* parser, AnnotableNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_no_init(true);
            } else {
                parser->error("couldn't make the struct def no_init");
            }
        }},
        { "anonymous", [](Parser* parser, AnnotableNode* node) -> void {
            if(!node->set_anonymous(true)) {
                parser->error("couldn't make the declaration anonymous");
            }
        }},
        { "extern", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_extern(true);
            } else {
                parser->error("couldn't make the function extern");
            }
        }},
        { "implicit", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_implicit(true);
            } else {
                parser->error("couldn't make the function implicit");
            }
        }},
        { "propagate", [](Parser* parser, AnnotableNode* node) -> void {
            const auto stmt = node->as_using_stmt();
            if(stmt) {
                stmt->set_propagate(true);
            } else {
                parser->error("couldn't make the using statement propagate");
            }
        }},
        { "direct_init", [](Parser* parser, AnnotableNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_direct_init(true);
            } else {
                parser->error("couldn't make the struct direct init");
            }
        }},
        { "no_return", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_noReturn(true);
            } else {
                parser->error("couldn't make the function no return");
            }
        }},
        { "cpp", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_cpp_mangle(true);
            } else {
                parser->error("couldn't make the function cpp");
            }
        }},
        { "clear", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_clear_fn(true);
            } else {
                parser->error("couldn't make the function a clear function");
            }
        }},
        { "copy", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_copy_fn(true);
            } else {
                parser->error("couldn't make the function a copy function");
            }
        }},
        { "deprecated", [](Parser* parser, AnnotableNode* node) -> void {
            if(!node->set_deprecated(true)) {
                parser->error("couldn't make the declaration deprecated");
            }
        }},
        { "move", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_move_fn(true);
            } else {
                parser->error("couldn't make the function a move function");
            }
        }},
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