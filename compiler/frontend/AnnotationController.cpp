// Copyright (c) Chemical Language Foundation 2025.

#include "AnnotationController.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/UsingStmt.h"

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

void annot_handler_inline_hint(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_hint = true;
    } else {
        parser->error("couldn't make the function inline hint");
    }
}

void annot_handler_compiler_inline(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.compiler_inline = true;
    } else {
        parser->error("couldn't make the function compiler inline");
    }
}

void annot_handler_size_opt(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.opt_size = true;
    } else {
        parser->error("couldn't make the function opt size");
    }
}

void annot_handler_size_min(Parser* parser, ASTNode* node) {
    auto func = node->as_function();
    if(func) {
        func->attrs.min_size = true;
    } else {
        parser->error("couldn't make the function min size");
    }
}

void annot_handler_compiler_interface(Parser* parser, ASTNode* node) {
    // we used to make these structs no_mangle by default
    // but now we don't, because now we put module name prefix
    // in the symbols we provide to the module using cbi
}

void annot_handler_no_mangle(Parser* parser, ASTNode* node) {
    if(!node->set_no_mangle(true)) {
        parser->error("couldn't make the node no_mangle");
    }
}

void annot_handler_constructor(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_constructor_fn(true);
    } else {
        parser->error("couldn't make the function constructor");
    }
}

void annot_handler_delete(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_delete_fn(true);
    } else {
        parser->error("couldn't make the function a delete function");
    }
}

void annot_handler_override(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_override(true);
    } else {
        parser->error("couldn't make the function override");
    }
}

void annot_handler_unsafe(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_unsafe(true);
    } else {
        parser->error("couldn't make the function unsafe");
    }
}

void annot_handler_no_init(Parser* parser, ASTNode* node) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_no_init(true);
    } else {
        parser->error("couldn't make the struct def no_init");
    }
}

void annot_handler_anonymous(Parser* parser, ASTNode* node) {
    if(!node->set_anonymous(true)) {
        parser->error("couldn't make the declaration anonymous");
    }
}

void annot_handler_extern(Parser* parser, ASTNode* node) {
    if(!node->set_no_mangle(true)) {
        parser->error("couldn't make the node no_mangle");
    }
    const auto func = node->as_function();
    if(func) {
        func->set_extern(true);
    }
}

void annot_handler_implicit(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_implicit(true);
    } else {
        parser->error("couldn't make the function implicit");
    }
}

void annot_handler_direct_init(Parser* parser, ASTNode* node) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_direct_init(true);
    } else {
        parser->error("couldn't make the struct direct init");
    }
}

void annot_handler_abstract(Parser* parser, ASTNode* node) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_abstract(true);
    } else {
        parser->error("couldn't make the struct abstract");
    }
}

void annot_handler_thread_local(Parser* parser, ASTNode* node) {
    if(node->kind() == ASTNodeKind::VarInitStmt && node->is_top_level()) {
        node->as_var_init_unsafe()->set_thread_local(true);
    } else {
        parser->error("cannot make the declaration thread local");
    }
}

void annot_handler_maxalign(Parser* parser, ASTNode* node) {
    // TODO
}

void annot_handler_no_return(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_noReturn(true);
    } else {
        parser->error("couldn't make the function no return");
    }
}

void annot_handler_cpp(Parser* parser, ASTNode* node) {
    const auto func = node->as_function();
    if(func) {
        func->set_cpp_mangle(true);
        // TODO since cpp mangle is not supported yet
        // we will set it to no mangle so C decl can be linked
        func->set_no_mangle(true);
    } else {
        parser->error("couldn't make the function cpp");
    }
}

void annot_handler_copy(Parser* parser, ASTNode* node) {
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
}

void annot_handler_clone(Parser* parser, ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            node->as_function_unsafe()->set_copy_fn(true);
            return;
        default:
            parser->error("unexpected clone annotation");
    }
}

void annot_handler_static(Parser* parser, ASTNode* node) {
    const auto interface = node->as_interface_def();
    if(interface) {
        interface->set_is_static(true);
    } else {
        parser->error("couldn't make the interface static");
    }
}

void annot_handler_deprecated(Parser* parser, ASTNode* node) {
    if(!node->set_deprecated(true)) {
        parser->error("couldn't make the declaration deprecated");
    }
}

AnnotationController::AnnotationController() {
    definitions = {
            { "inline", { annot_handler_inline, AnnotationDefType::Handler } },
            { "inline:always", { annot_handler_inline_always, AnnotationDefType::Handler } },
            { "inline.always", { annot_handler_inline_always, AnnotationDefType::Handler } },
            { "noinline", { annot_handler_noinline, AnnotationDefType::Handler } },
            { "inline:no", { annot_handler_noinline, AnnotationDefType::Handler } },
            { "inline:hint", { annot_handler_inline_hint, AnnotationDefType::Handler } },
            { "compiler.inline", { annot_handler_compiler_inline, AnnotationDefType::Handler } },
            { "size:opt", { annot_handler_size_opt, AnnotationDefType::Handler } },
            { "size:min", { annot_handler_size_min, AnnotationDefType::Handler } },
            { "compiler.interface", { annot_handler_compiler_interface, AnnotationDefType::Handler } },
            { "no_mangle", { annot_handler_no_mangle, AnnotationDefType::Handler } },
            { "export", { annot_handler_no_mangle, AnnotationDefType::Handler } },
            { "constructor", { annot_handler_constructor, AnnotationDefType::Handler } },
            { "make", { annot_handler_constructor, AnnotationDefType::Handler } },
            { "delete", { annot_handler_delete, AnnotationDefType::Handler } },
            { "override", { annot_handler_override, AnnotationDefType::Handler } },
            { "unsafe", { annot_handler_unsafe, AnnotationDefType::Handler } },
            { "no_init", { annot_handler_no_init, AnnotationDefType::Handler } },
            { "anonymous", { annot_handler_anonymous, AnnotationDefType::Handler } },
            { "extern", { annot_handler_extern, AnnotationDefType::Handler } },
            { "implicit", { annot_handler_implicit, AnnotationDefType::Handler } },
            { "direct_init", { annot_handler_direct_init, AnnotationDefType::Handler } },
            { "abstract", { annot_handler_abstract, AnnotationDefType::Handler } },
            { "thread_local", { annot_handler_thread_local, AnnotationDefType::Handler } },
            { "maxalign", { annot_handler_maxalign, AnnotationDefType::Handler } },
            { "no_return", { annot_handler_no_return, AnnotationDefType::Handler } },
            { "cpp", { annot_handler_cpp, AnnotationDefType::Handler } },
            { "copy", { annot_handler_copy, AnnotationDefType::Handler } },
            { "clone", { annot_handler_clone, AnnotationDefType::Handler } },
            { "static", { annot_handler_static, AnnotationDefType::Handler } },
            { "deprecated", { annot_handler_deprecated, AnnotationDefType::Handler } },
    };

    // reserving memory for faster operations
    definitions.reserve(128);
    collections.reserve(32);
    marked.reserve(128);

}