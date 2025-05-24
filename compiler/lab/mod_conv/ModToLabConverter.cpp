// Copyright (c) Chemical Language Foundation 2025.

#include "ModToLabConverter.h"
#include "compiler/processor/ModuleFileData.h"
#include "ast/statements/Import.h"

void writeWithSep(std::vector<chem::string_view>& list, char sep, std::ostream& output) {
    unsigned i = 0;
    const auto total = list.size();
    const auto last = total - 1;
    while(i < total) {
        output << list[i];
        if(i != last) {
            output << sep;
        }
        i++;
    }
}

void writeAsIdentifier(ImportStatement* stmt, std::ostream& output) {
    if(!stmt->as_identifier.empty()) {
        output << "__mod_";
        output << stmt->as_identifier;
    } else {
        output << "__mod_";
        writeWithSep(stmt->identifier, '_', output);
    }
}

void convertToBuildLab(const ModuleFileData& data, std::ostream& output) {

    // writing imports for modules
    output << "import lab;\n";

    // writing imports for dependencies
    for(const auto node : data.scope.body.nodes) {
        // for each import statement, we emit a statement
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                output << "import \"@";
                writeWithSep(stmt->identifier, ':', output);
                output << "/build.lab\" as ";
                writeAsIdentifier(stmt, output);
                output << '\n';
                break;
            }
            default:
#ifdef DEBUG
                throw std::runtime_error("expected import statement");
#endif
                continue;
        }
    }

    // build method
    output << "\nfunc build(ctx : *mut BuildContext) : *mut Module {\n";
    output << "\tconst mod = ctx.new_module(\"" << data.scope_name << "\", \"" << data.module_name << "\", [ ";
    // calling get functions on dependencies
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                writeAsIdentifier(stmt, output);
                output << ".get(ctx), ";
                break;
            }
            default:
                continue;
        }
    }
    output << "]);\n";

    if(!data.sources_list.empty()) {
        for(auto& src : data.sources_list) {
            const auto has_if = !src.if_condition.empty();
            if(has_if) {
                output << "if(def." << src.if_condition << ") {\n";
            }
            output << "\t\tctx.add_path(mod, lab::rel_path_to(\"" << src.path << "\").to_view());\n";
            if(has_if) {
                output << "\t}\n";
            }
        }
    }

    if(!data.compiler_interfaces.empty()) {
        // writing each compiler interface
        for(const auto interface : data.compiler_interfaces) {
            output << "\tctx.add_compiler_interface(mod, \"" << interface << "\");\n";
        }
    }
    // TODO: enable this when bug with automatic constructor call is fixed
//    if(!data.compiler_interfaces.empty()) {
//        output << "\tctx.add_compiler_interfaces(mod, [ ";
//        // writing each compiler interface
//        for(const auto interface : data.compiler_interfaces) {
//            output << '"' << interface << "\", ";
//        }
//        output << "]);\n";
//    }
    output << "\treturn mod;\n";
    output << "}\n\n";

    // get method
    output << "var __chx_should_build : bool = true;\n";
    output << "var __chx_cached_build : *mut Module = null;\n";
    output << "public func get(ctx : *mut BuildContext) : *mut Module {\n";
    output << "\treturn ctx.default_get(&__chx_should_build, &__chx_cached_build, build);\n";
    output << "}\n";

}