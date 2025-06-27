// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "compiler/cbi/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/UsingStmt.h"

bool make_node_no_mangle(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            node->as_function_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::InterfaceDecl:
            node->as_interface_def_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::StructDecl:
            node->as_struct_def_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::UnionDecl:
            node->as_union_def_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::VariantDecl:
            node->as_variant_def_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::TypealiasStmt:
            node->as_typealias_unsafe()->set_no_mangle(true);
            return true;
        case ASTNodeKind::VarInitStmt:
            node->as_var_init_unsafe()->set_no_mangle(true);
            return true;
        default:
            return false;
    }
}

const std::unordered_map<chem::string_view, const AnnotationModifierFunc> AnnotationModifierFunctions = {
        { "inline", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.is_inline = true;
            } else {
                parser->error("couldn't make the function inline");
            }
        } },
        { "inline:always", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.always_inline = true;
            } else {
                parser->error("couldn't make the function inline always");
            }
        } },
        { "noinline", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.no_inline = true;
            } else {
                parser->error("couldn't make the function noinline");
            }
        } },
        { "inline:no", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.no_inline = true;
            } else {
                parser->error("couldn't make the function noinline");
            }
        } },
        { "inline:hint", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.inline_hint = true;
            } else {
                parser->error("couldn't make the function inline hint");
            }
        } },
        { "compiler.inline", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.compiler_inline = true;
            } else {
                parser->error("couldn't make the function compiler inline");
            }
        } },
        { "size:opt", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.opt_size = true;
            } else {
                parser->error("couldn't make the function opt size");
            }
        } },
        { "size:min", [](Parser* parser, ASTNode* node) -> void {
            auto func = node->as_function();
            if(func) {
                func->attrs.min_size = true;
            } else {
                parser->error("couldn't make the function min size");
            }
        } },
        { "comptime", [](Parser* parser, ASTNode* node) -> void {
            if(!node->set_comptime(true)) {
                parser->error("couldn't make the declaration comptime");
            }
        } },
        { "compiler.interface", [](Parser* parser, ASTNode* node) -> void {
            // we used to make these structs no_mangle by default
            // but now we don't, because now we put module name prefix
            // in the symbols we provide to the module using cbi
        } },
        { "no_mangle", [](Parser* parser, ASTNode* node) -> void {
            if(!make_node_no_mangle(node)) {
                parser->error("couldn't make the node no_mangle");
            }
        } },
        { "export", [](Parser* parser, ASTNode* node) -> void {
            if(!make_node_no_mangle(node)) {
                parser->error("couldn't make the node no_mangle");
            }
        } },
        { "constructor", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_constructor_fn(true);
            } else {
                parser->error("couldn't make the function constructor");
            }
        } },
        { "make", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_constructor_fn(true);
            } else {
                parser->error("couldn't make the function constructor");
            }
        } },
        { "delete", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_delete_fn(true);
            } else {
                parser->error("couldn't make the function a delete function");
            }
        } },
        { "override", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_override(true);
            } else {
                parser->error("couldn't make the function override");
            }
        } },
        { "unsafe", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_unsafe(true);
            } else {
                parser->error("couldn't make the function unsafe");
            }
        } },
        { "no_init", [](Parser* parser, ASTNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_no_init(true);
            } else {
                parser->error("couldn't make the struct def no_init");
            }
        }},
        { "anonymous", [](Parser* parser, ASTNode* node) -> void {
            if(!node->set_anonymous(true)) {
                parser->error("couldn't make the declaration anonymous");
            }
        }},
        { "extern", [](Parser* parser, ASTNode* node) -> void {
            if(!make_node_no_mangle(node)) {
                parser->error("couldn't make the node no_mangle");
            }
            const auto func = node->as_function();
            if(func) {
                func->set_extern(true);
            }
        }},
        { "implicit", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_implicit(true);
            } else {
                parser->error("couldn't make the function implicit");
            }
        }},
        { "propagate", [](Parser* parser, ASTNode* node) -> void {
            const auto stmt = node->as_using_stmt();
            if(stmt) {
                stmt->set_propagate(true);
            } else {
                parser->error("couldn't make the using statement propagate");
            }
        }},
        { "direct_init", [](Parser* parser, ASTNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_direct_init(true);
            } else {
                parser->error("couldn't make the struct direct init");
            }
        }},
        { "abstract", [](Parser* parser, ASTNode* node) -> void {
            const auto def = node->as_struct_def();
            if(def) {
                def->set_abstract(true);
            } else {
                parser->error("couldn't make the struct abstract");
            }
        }},
        { "maxalign", [](Parser* parser, ASTNode* node) -> void {
            // TODO
        }},
        { "no_return", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_noReturn(true);
            } else {
                parser->error("couldn't make the function no return");
            }
        }},
        { "cpp", [](Parser* parser, ASTNode* node) -> void {
            const auto func = node->as_function();
            if(func) {
                func->set_cpp_mangle(true);
                // TODO since cpp mangle is not supported yet
                // we will set it to no mangle so C decl can be linked
                func->set_no_mangle(true);
            } else {
                parser->error("couldn't make the function cpp");
            }
        }},
        { "copy", [](Parser* parser, ASTNode* node) -> void {
            switch(node->kind()) {
                case ASTNodeKind::FunctionDecl:
                    node->as_function_unsafe()->set_copy_fn(true);
                    return;
                case ASTNodeKind::StructDecl:
                    node->as_struct_def_unsafe()->set_shallow_copyable(true);
                    return;
                case ASTNodeKind::UnionDecl:
                    node->as_union_def_unsafe()->set_shallow_copyable(true);
                    return;
                case ASTNodeKind::VariantDecl:
                    node->as_variant_def_unsafe()->set_shallow_copyable(true);
                    return;
                default:
                    parser->error("unexpected copy annotation");
            }
        }},
        { "clone", [](Parser* parser, ASTNode* node) -> void {
            switch(node->kind()) {
                case ASTNodeKind::FunctionDecl:
                    node->as_function_unsafe()->set_copy_fn(true);
                    return;
                default:
                    parser->error("unexpected clone annotation");
            }
        }},
        { "static", [](Parser* parser, ASTNode* node) -> void {
            const auto interface = node->as_interface_def();
            if(interface) {
                interface->set_is_static(true);
            } else {
                parser->error("couldn't make the interface static");
            }
        }},
        { "deprecated", [](Parser* parser, ASTNode* node) -> void {
            if(!node->set_deprecated(true)) {
                parser->error("couldn't make the declaration deprecated");
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
        annotations.emplace_back(name_view, found->second);
        return true;
    } else {
        error() << "unknown annotation found '" << annot->value << "'";
        return true;
    }
}

Value* Parser::parseMacroValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::HashMacro) {
        const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
        auto found = binder->findHook(view, CBIFunctionType::ParseMacroValue);
        if(found) {
            token++;
            return (UserParserParseMacroValueFn (found))(this, &allocator);
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
            return (UserParserParseMacroNodeFn (found))(this, &allocator);
        } else {
            error() << "couldn't find macro parser for '" << t.value << "'";
        }
    }
    return nullptr;
}