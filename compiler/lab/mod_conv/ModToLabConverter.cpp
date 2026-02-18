// Copyright (c) Chemical Language Foundation 2025.

#include "ModToLabConverter.h"
#include "compiler/processor/ModuleFileData.h"
#include "ast/statements/Import.h"
#include "std/except.h"

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

void writeIfConditional(ModFileIfBase* if_base, std::ostream& output) {
    if(if_base == nullptr) return;
    if(if_base->is_id) {
        const auto if_id = (ModFileIfId*) if_base;
        if(if_id->is_negative) {
            output << '!';
        }
        output << "__chx_job.getTarget().";
        output << if_id->value;
    } else {
        const auto if_expr = (ModFileIfExpr*) if_base;
        output << '(';
        writeIfConditional(if_expr->left, output);
        output << (if_expr->op == ModFileIfExprOp::And ? " && " : " || ");
        writeIfConditional(if_expr->right, output);
        output << ')';
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
                if(!stmt->version.empty() || !stmt->subdir.empty()) {
                    // skip remote imports here
                    continue;
                }
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
                CHEM_THROW_RUNTIME("expected import statement");
#endif
                continue;
        }
        i++;
    }

    // build method
    output << "\npublic func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {\n";

    output << "\tconst __chx_already_exists = ctx.get_cached(__chx_job, \"" << data.scope_name << "\", \"" << data.module_name << "\");\n";
    output << "\tif(__chx_already_exists != null) { return __chx_already_exists; }\n";
    
    // Create module first so we can add dependencies later
    // We cannot create module with dependencies array directly if we have remote imports that need to be fetched and added
    // So we will create module with empty array and then add dependencies?
    // Wait, `new_module` takes dependencies array.
    // If we have remote imports, we need to fetch them. The fetch function takes `mod` pointer.
    // So we need `mod` to call `fetch_mod_dependency`.
    // But `new_module` needs `deps`.
    // We can collect local ops first.
    // But remote imports are also modules.
    // If we fetch them, `fetch_mod_dependency` adds them to `job->remote_imports`.
    // They are NOT added to the module dependencies immediately.
    // `process_remote_imports` downloads them and THEN adds them to the module dependencies.
    // So we can create the module with local dependencies, and then call fetch for remote ones.
    
    output << "\tconst deps : []*mut Module = [ ";

    i = 0;
    unsigned deps_size = 0;
    // calling get functions on dependencies
    // identifiers for remote imports: import a from "..." -> a
    // we need to know which ones are remote. 
    // We iterate again.
    
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                if(!stmt->version.empty() || !stmt->subdir.empty()) {
                    // remote import, skip adding to deps list for now
                } else {
                    // local import
                    writeAsIdentifier(stmt, i, output);
                    output << ".build(ctx, __chx_job), ";
                    deps_size++;
                }
                break;
            }
            default:
                break;
        }
        i++;
    }

    output << " ];\n";
    output << "\tconst mod = ctx.new_module(\"" << data.scope_name << "\", \"" << data.module_name << "\", std::span<*Module>(deps, " << deps_size << "));\n";
    output << "\tctx.set_cached(__chx_job, mod)\n";
    
    // Now handle remote imports
    i = 0;
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                if(!stmt->version.empty() || !stmt->subdir.empty()) {
                    // remote import
                    // Generate: var __imp_repo_X = ImportRepo(...);
                    // ctx.fetch_mod_dependency(__chx_job, mod, &__imp_repo_X);
                    
                    std::string id_str;
                    if(!stmt->as_identifier.empty()) {
                         id_str = stmt->as_identifier.str();
                    } else if(stmt->identifier.empty()) {
                         // identifier list is empty but file path present?
                         // "import ... from ..."
                         // If no identifier, how do we use it?
                         // "import 'path'"?
                         // Assuming there is an identifier for `as`.
                         // If not, we generate a unique ID based on index.
                         id_str = "__mod_" + std::to_string(i) + "_stmt"; // Or just use index
                    } else {
                        // use first identifier?
                         if(stmt->identifier.size() == 1) {
                             id_str = stmt->identifier[0].str();
                         } else {
                             // complex import?
                             id_str = "__mod_" + std::to_string(i);
                         }
                    }
                    
                    // Actually `ImportRepo` needs `id`. This `id` is the module name/id in `RemoteImport`.
                    // User said: "identifier is the actual identifier".
                    // If `import glib from ...`, id is `glib`.
                    
                    if(!stmt->identifier.empty()) {
                         id_str = stmt->identifier[0].str();
                    } else if(!stmt->as_identifier.empty()){
                         id_str = stmt->as_identifier.str();
                    } else {
                         id_str = "remote_" + std::to_string(i);
                    }

                    output << "\tvar __imp_repo_" << i << " = ImportRepo(\n";
                    output << "\t\t\"" << id_str << "\",\n"; // id
                    output << "\t\t\"" << stmt->filePath << "\",\n"; // from
                    output << "\t\t\"" << stmt->subdir << "\",\n"; // subdir
                    output << "\t\t\"" << stmt->version << "\"\n"; // version
                    output << "\t);\n";
                    output << "\tctx.fetch_mod_dependency(__chx_job, mod, &__imp_repo_" << i << ");\n";
                }
                break;
            }
            default:
                break;
        }
        i++;
    }


    if(!data.sources_list.empty()) {
        for(auto& src : data.sources_list) {
            const auto has_if = src.if_cond != nullptr;
            if(has_if) {
                output << "\tif(";
                writeIfConditional(src.if_cond, output);
                output << ") {\n\t";
            }
            output << "\tctx.add_path(mod, lab::rel_path_to(\"" << src.path << "\").to_view());\n";
            if(has_if) {
                output << "\t}\n";
            }
        }
    }

    if(!data.link_libs.empty()) {
        for(auto& lib : data.link_libs) {
            const auto has_if = lib.if_cond != nullptr;
            if(has_if) {
                output << "\tif(";
                writeIfConditional(lib.if_cond, output);
                output << ") {\n\t";
            }
            output << "\tctx.link_system_lib(__chx_job, \"" << lib.name << "\", mod)\n";
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