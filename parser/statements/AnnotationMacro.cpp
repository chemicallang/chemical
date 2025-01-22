// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "ast/base/AnnotableNode.h"
#include "parser/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
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
        { "postmove", [](Parser* parser, AnnotableNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_post_move_fn(true);
            } else {
                parser->error("couldn't make the function a postmove function");
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
        { "static", [](Parser* parser, AnnotableNode* node) -> void {
            const auto interface = node->as_interface_def();
            if(interface) {
                interface->set_is_static(true);
            } else {
                parser->error("couldn't make the interface static");
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

bool Parser::parseAnnotation(ASTAllocator& allocator) {
    if(token->type != TokenType::Annotation) {
        return false;
    }
    const auto annot = token;
    token++;
    auto name_view = chem::string_view(annot->value.data() + 1, annot->value.size() - 1);
    auto found = AnnotationModifierFunctions.find(name_view);
    if(found != AnnotationModifierFunctions.end()) {
        annotations.emplace_back(found->second);
        return true;
    } else {
        error("unknown annotation found '" + annot->value.str() + "'");
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

ASTNode* Parser::parseMacroNode(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        auto& map = binder->parseMacroNodeFunctions;
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