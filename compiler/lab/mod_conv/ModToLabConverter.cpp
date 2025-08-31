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

void writeAsIdentifier(ImportStatement* stmt, unsigned index, std::ostream& output) {
    if(!stmt->as_identifier.empty()) {
        output << "__mod_";
        output << stmt->as_identifier;
    } else {
        if(stmt->identifier.empty()) {
            output << "__mod_" << index << "_stmt";
        } else {
            output << "__mod_";
            writeWithSep(stmt->identifier, '_', output);
        }
    }
}

void convertToBuildLab(const ModuleFileData& data, std::ostream& output) {

    // writing imports for modules
    output << "import lab;\n";
    output << "import std;\n";

    unsigned i = 0;

    // writing imports for dependencies
    for(const auto node : data.scope.body.nodes) {
        // for each import statement, we emit a statement
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                if(stmt->filePath.empty()) {
                    output << "import \"@";
                    writeWithSep(stmt->identifier, ':', output);
                    output << "/build.lab\" as ";
                    writeAsIdentifier(stmt, i, output);
                    output << '\n';
                } else {
                    output << "import \"";
                    output << stmt->filePath;
                    output << "/build.lab\" as ";
                    writeAsIdentifier(stmt, i, output);
                    output << '\n';
                }
                break;
            }
            default:
#ifdef DEBUG
                throw std::runtime_error("expected import statement");
#endif
                continue;
        }
        i++;
    }

    // build method
    output << "\npublic func build(ctx : *mut BuildContext, job : *LabJob) : *mut Module {\n";

    output << "\tconst __chx_already_exists = ctx.get_cached(job, \"" << data.scope_name << "\", \"" << data.module_name << "\");\n";
    output << "\tif(__chx_already_exists != null) { return __chx_already_exists; }\n";

    output << "\tconst mod = ctx.new_module(\"" << data.scope_name << "\", \"" << data.module_name << "\", [ ";

    i = 0;
    // calling get functions on dependencies
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                writeAsIdentifier(stmt, i, output);
                output << ".build(ctx, job), ";
                break;
            }
            default:
                break;
        }
        i++;
    }
    output << "]);\n";
    output << "\tctx.set_cached(job, mod)\n";

    if(!data.sources_list.empty()) {
        for(auto& src : data.sources_list) {
            const auto has_if = !src.if_condition.empty();
            if(has_if) {
                output << "if(";
                if(src.is_negative) {
                    output << '!';
                }
                output << "job.target." << src.if_condition << ") {\n";
            }
            output << "\tctx.add_path(mod, lab::rel_path_to(\"" << src.path << "\").to_view());\n";
            if(has_if) {
                output << "\t}\n";
            }
        }
    }

    if(!data.link_libs.empty()) {
        for(auto& lib : data.link_libs) {
            const auto has_if = !lib.if_condition.empty();
            if(has_if) {
                output << "if(";
                if(lib.is_negative) {
                    output << '!';
                }
                output << "job.target." << lib.if_condition << ") {\n";
            }
            output << "\tctx.link_system_lib(mod, \"" << lib.name << "\")\n";
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

// get method is not available
//    // get method
//    output << "var __chx_should_build : bool = true;\n";
//    output << "var __chx_cached_build : *mut Module = null;\n";
//    output << "public func get(ctx : *mut BuildContext) : *mut Module {\n";
//    output << "\treturn ctx.default_get(&mut __chx_should_build, &mut __chx_cached_build, build);\n";
//    output << "}\n";

}