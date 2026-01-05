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

void annot_handler_inline(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::InlineHint;
    } else {
        parser->error("couldn't make the function inline");
    }
}

void annot_handler_inline_always(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::AlwaysInline;
    } else {
        parser->error("couldn't make the function inline always");
    }
}

void annot_handler_noinline(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::NoInline;
    } else {
        parser->error("couldn't make the function noinline");
    }
}

void annot_handler_compiler_inline(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::CompilerInline;
    } else {
        parser->error("couldn't make the function compiler inline");
    }
}

void annot_handler_size_opt(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::OptSize;
    } else {
        parser->error("couldn't make the function opt size");
    }
}

void annot_handler_size_min(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    auto func = node->as_function();
    if(func) {
        func->attrs.inline_strategy = InlineStrategy::MinSize;
    } else {
        parser->error("couldn't make the function min size");
    }
}

void annot_handler_compiler_interface(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    // we used to make these structs no_mangle by default
    // but now we don't, because now we put module name prefix
    // in the symbols we provide to the module using cbi
}

void annot_handler_no_mangle(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(!node->set_no_mangle(true)) {
        parser->error("couldn't make the node no_mangle");
    }
}

void annot_handler_constructor(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_constructor_fn(true);
    } else {
        parser->error("couldn't make the function constructor");
    }
}

void annot_handler_delete(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_delete_fn(true);
    } else {
        parser->error("couldn't make the function a delete function");
    }
}

void annot_handler_override(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_override(true);
    } else {
        parser->error("couldn't make the function override");
    }
}

void annot_handler_unsafe(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_unsafe(true);
    } else {
        parser->error("couldn't make the function unsafe");
    }
}

void annot_handler_stdcall(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_std_call(true);
    } else {
        parser->error("couldn't make the function stdcall");
    }
}

void annot_handler_dllimport(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_dll_import(true);
    } else {
        parser->error("couldn't make the function dllimport");
    }
};

void annot_handler_dllexport(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_dll_export(true);
    } else {
        parser->error("couldn't make the function dllexport");
    }
}

void annot_handler_no_init(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_no_init(true);
    } else {
        parser->error("couldn't make the struct def no_init");
    }
}

void annot_handler_anonymous(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(!node->set_anonymous(true)) {
        parser->error("couldn't make the declaration anonymous");
    }
}

void annot_handler_const(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    // TODO: not yet implemented
}

void annot_handler_extern(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(!node->set_no_mangle(true)) {
        parser->error("couldn't make the node no_mangle");
    }
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            node->as_function_unsafe()->set_extern(true);
            return;
        case ASTNodeKind::StructDecl:
            node->as_struct_def_unsafe()->set_extern(true);
            return;
    }
}

void annot_handler_implicit(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_implicit(true);
    } else {
        parser->error("couldn't make the function implicit");
    }
}

void annot_handler_direct_init(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_direct_init(true);
    } else {
        parser->error("couldn't make the struct direct init");
    }
}

void annot_handler_abstract(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto def = node->as_struct_def();
    if(def) {
        def->set_abstract(true);
    } else {
        parser->error("couldn't make the struct abstract");
    }
}

void annot_handler_thread_local(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(node->kind() == ASTNodeKind::VarInitStmt && node->is_top_level()) {
        node->as_var_init_unsafe()->set_thread_local(true);
    } else {
        parser->error("cannot make the declaration thread local");
    }
}

void annot_handler_maxalign(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    // TODO
}

void annot_handler_no_return(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto func = node->as_function();
    if(func) {
        func->set_noReturn(true);
    } else {
        parser->error("couldn't make the function no return");
    }
}

void annot_handler_cpp(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
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

void annot_handler_copy(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            node->as_function_unsafe()->set_copy_fn(true);
            return;
        default:
            parser->error("unexpected copy annotation");
    }
}

void annot_handler_clone(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            node->as_function_unsafe()->set_copy_fn(true);
            return;
        default:
            parser->error("unexpected clone annotation");
    }
}

void annot_handler_static(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    const auto interface = node->as_interface_def();
    if(interface) {
        interface->set_is_static(true);
    } else {
        parser->error("couldn't make the interface static");
    }
}

void annot_handler_deprecated(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(!node->set_deprecated(true)) {
        parser->error("couldn't make the declaration deprecated");
    }
}

void annot_handler_align(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    // TODO:
}

AnnotationController::AnnotationController() {

    // initialize intrinsic annotations
    definitions = {
            { "inline", { annot_handler_inline, "inline", AnnotationDefType::Handler } },
            { "inline.always", { annot_handler_inline_always, "inline.always", AnnotationDefType::Handler } },
            { "noinline", { annot_handler_noinline, "noinline", AnnotationDefType::Handler } },
            { "inline.no", { annot_handler_noinline, "inline.no", AnnotationDefType::Handler } },
            { "compiler.inline", { annot_handler_compiler_inline, "compiler.inline", AnnotationDefType::Handler } },
            { "size:opt", { annot_handler_size_opt, "size:opt", AnnotationDefType::Handler } },
            { "size:min", { annot_handler_size_min, "size:min", AnnotationDefType::Handler } },
            { "compiler.interface", { annot_handler_compiler_interface, "compiler.interface", AnnotationDefType::Handler } },
            { "no_mangle", { annot_handler_no_mangle, "no_mangle", AnnotationDefType::Handler } },
            { "export", { annot_handler_no_mangle, "export", AnnotationDefType::Handler } },
            { "constructor", { annot_handler_constructor, "constructor", AnnotationDefType::Handler } },
            { "make", { annot_handler_constructor, "make", AnnotationDefType::Handler } },
            { "delete", { annot_handler_delete, "delete", AnnotationDefType::Handler } },
            { "override", { annot_handler_override, "override", AnnotationDefType::Handler } },
            { "unsafe", { annot_handler_unsafe, "unsafe", AnnotationDefType::Handler } },
            { "stdcall", { annot_handler_stdcall, "stdcall", AnnotationDefType::Handler } },
            { "dllimport", { annot_handler_dllimport, "dllimport", AnnotationDefType::Handler } },
            { "dllexport", { annot_handler_dllexport, "dllexport", AnnotationDefType::Handler } },
            { "no_init", { annot_handler_no_init, "no_init", AnnotationDefType::Handler } },
            { "anonymous", { annot_handler_anonymous, "anonymous", AnnotationDefType::Handler } },
            { "const", { annot_handler_const, "const", AnnotationDefType::Handler } },
            { "extern", { annot_handler_extern, "extern", AnnotationDefType::Handler } },
            { "implicit", { annot_handler_implicit, "implicit", AnnotationDefType::Handler } },
            { "direct_init", { annot_handler_direct_init, "direct_init", AnnotationDefType::Handler } },
            { "abstract", { annot_handler_abstract, "abstract", AnnotationDefType::Handler } },
            { "thread_local", { annot_handler_thread_local, "thread_local", AnnotationDefType::Handler } },
            { "maxalign", { annot_handler_maxalign, "maxalign", AnnotationDefType::Handler } },
            { "no_return", { annot_handler_no_return, "no_return", AnnotationDefType::Handler } },
            { "cpp", { annot_handler_cpp, "cpp", AnnotationDefType::Handler } },
            { "copy", { annot_handler_copy, "copy", AnnotationDefType::Handler } },
            { "clone", { annot_handler_clone, "clone", AnnotationDefType::Handler } },
            { "static", { annot_handler_static, "static", AnnotationDefType::Handler } },
            { "deprecated", { annot_handler_deprecated, "deprecated", AnnotationDefType::Handler } },
            { "align", { annot_handler_align, "align", AnnotationDefType::Handler } },
    };

    // reserving memory for faster operations
    definitions.reserve(128);
    collections.reserve(32);
    marked.reserve(128);
    single_marked.reserve(32);

    // adding testing annotations
    create_collector_annotation("test", 0);
    create_single_marker_annotation("test.before_each");
    create_single_marker_annotation("test.after_each");
    create_marker_annotation("test.id");
    create_marker_annotation("test.name");
    create_marker_annotation("test.group");
    create_marker_annotation("test.pass_on_crash");
    create_marker_annotation("test.ignore");
    create_marker_annotation("test.timeout");
    create_marker_annotation("test.async");
    create_marker_annotation("test.retry");
    create_marker_annotation("test.benchmark");


}

void AnnotationController::ensure_test_resources() {
    get_collection(get_definition("test")->collection_id).nodes.reserve(512);
}

void AnnotationController::mark_single(Parser& parser, ASTNode* node, AnnotationDefinition& definition, std::vector<Value*>& arguments) {
    switch(definition.policy) {
        case SingleMarkerMultiplePolicy::Override:
            mark_single_emplace(node, definition, arguments);
            return;
        case SingleMarkerMultiplePolicy::Ignore:
            if(!single_marked.contains(definition.name)) {
                mark_single_emplace(node, definition, arguments);
            }
            return;
        case SingleMarkerMultiplePolicy::Error:
            if(single_marked.contains(definition.name)) {
                parser.error() << "single annotation with name '" << definition.name << "' already marks an existing declaration";
            } else {
                mark_single_emplace(node, definition, arguments);
            }
            return;
    }
}