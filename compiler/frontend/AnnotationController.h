// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "std/chem_string_view.h"

class ASTNode;
class Value;
class Parser;
class AnnotationController;

enum class AnnotationDefType {
    /**
     * the annotation is intrinsic to compiler, it handles itself
     */
    Intrinsic,
    /**
     * marks a node, so you can quickly check whether a node has been marked
     * with a given annotation
     */
    Marker,
    /**
     * collects node into a known collection
     */
    Collector,
    /**
     * marks the node and collects into a known collection
     */
    MarkerAndCollector,
};

struct AnnotationDefinition {

    void(*handler)(Parser* parser, ASTNode* node);

    AnnotationDefType type;

};

void annot_handler_inline(Parser* parser, ASTNode* node);
void annot_handler_inline_always(Parser* parser, ASTNode* node);
void annot_handler_noinline(Parser* parser, ASTNode* node);

void annot_handler_inline(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.is_inline = true;
    } else {
        parser->error("couldn't make the function inline");
    }
}

void annot_handler_inline_always(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.always_inline = true;
    } else {
        parser->error("couldn't make the function inline always");
    }
}

void annot_handler_noinline(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.no_inline = true;
    } else {
        parser->error("couldn't make the function noinline");
    }
}

class AnnotationController {
private:

    std::unordered_map<chem::string_view, AnnotationDefinition> definitions;

public:

    void mark(ASTNode* node, chem::string_view& name, std::vector<Value*>& arguments);

    void collect(ASTNode* node, chem::string_view& name, std::vector<Value*>& arguments);

    void mark_or_collect(ASTNode* node, chem::string_view& name, std::vector<Value*>& arguments);

    AnnotationController() {
        definitions = {
                { "inline", { annot_handler_inline, AnnotationDefType::Intrinsic } },
                { "inline:always", { annot_handler_inline, AnnotationDefType::Intrinsic } },
                { "inline.always", { annot_handler_inline, AnnotationDefType::Intrinsic } },
                { "noinline", { annot_handler_inline, AnnotationDefType::Intrinsic } },
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
                { "thread_local", [](Parser* parser, ASTNode* node) -> void {
                    if(node->kind() == ASTNodeKind::VarInitStmt && node->is_top_level()) {
                        node->as_var_init_unsafe()->set_thread_local(true);
                    } else {
                        parser->error("cannot make the declaration thread local");
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
    }

};
