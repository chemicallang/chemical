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
    if(!stmt->getTopLevelAlias().empty()) {
        output << "__mod_";
        output << stmt->getTopLevelAlias();
    } else {
        //if(stmt->identifier.empty()) {
            output << "__mod_" << index << "_stmt";
        //} else {
        //    output << "__mod_";
        //    writeWithSep(stmt->identifier, '_', output);
        //}
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

void writeSymbolInfo(std::ostream& output, ImportStatement* stmt, const chem::string_view& indent) {

    auto alias = stmt->getTopLevelAlias();
    auto& items = stmt->getImportItems();
    if(alias.empty() && items.empty()) {
        output << "info: null";
        return;
    }

    output << "info: &DependencySymbolInfo {\n";
    output << indent << "\talias: \"" << alias << "\",\n"; // alias

    if(items.empty()) {
        output << indent << "\tsymbols : std::span<ImportSymbol>(),\n";
    } else {
        output << indent << "\tsymbols: [ ";
        for(auto& sym : items) {
            output << "ImportSymbol { parts : ";
            if(sym.parts.empty()) {
                output << "std::span<std::string_view>(), ";
            } else {
                output << "[ ";
                for (auto& part: sym.parts) {
                    output << "std::string_view(\"" << part << "\"),";
                }
                output << "], ";
            }
            output << "alias: \"" << sym.alias << "\" }, ";
        }
        output << " ],\n";
    }
    output << indent << "\tlocation: intrinsics::get_raw_location(),\n";
    output << indent << "}";
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
                if(stmt->isRemoteImport()) {
                    // skip remote imports here
                    continue;
                }
                if(stmt->isNativeLibImport()) {
                    output << "import \"@" << stmt->getSourcePath() << "/build.lab\" as ";
                    writeAsIdentifier(stmt, i, output);
                    output << '\n';
                } else {
                    output << "import \"" << stmt->getSourcePath() << "/build.lab\" as ";
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

    output << "\tconst deps : []ModuleDependency = [ ";

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
                if(stmt->isRemoteImport()) {
                    // remote import, skip adding to deps list for now
                    continue;
                }
                // local import
                output << "ModuleDependency { module: ";
                writeAsIdentifier(stmt, i, output);
                output << ".build(ctx, __chx_job), ";
                writeSymbolInfo(output, stmt, "\t\t");
                output << " }, ";
                deps_size++;
                break;
            }
            default:
                break;
        }
        i++;
    }

    output << " ];\n";
    const auto pkg_kind_str = (data.package_kind == PackageKind::Application) ? "PackageKind.Application" : "PackageKind.Library";
    output << "\tconst mod = ctx.new_package(ModuleType.Directory, " << pkg_kind_str << ", \"" << data.scope_name << "\", \"" << data.module_name << "\", std::span<ModuleDependency>(deps, " << deps_size << "));\n";
    output << "\tctx.set_cached(__chx_job, mod)\n";
    
    // Now handle remote imports
    i = 0;
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                if(stmt->isRemoteImport()) {
                    // remote import
                    // ctx.fetch_mod_dependency(__chx_job, mod, ImportRepo{...});
                    output << "\tctx.fetch_mod_dependency(__chx_job, mod, ImportRepo {\n";
                    output << "\t\tfrom: \"" << stmt->getSourcePath() << "\",\n"; // from
                    if(!stmt->getSubdir().empty()) output << "\t\tsubdir: \"" << stmt->getSubdir() << "\",\n"; // subdir
                    if(!stmt->getVersion().empty()) output << "\t\tversion: \"" << stmt->getVersion() << "\",\n"; // version
                    if(!stmt->getBranch().empty()) output << "\t\tbranch: \"" << stmt->getBranch() << "\",\n"; // branch
                    if(!stmt->getCommit().empty()) output << "\t\tcommit: \"" << stmt->getCommit() << "\",\n"; // commit

                    auto alias = stmt->getTopLevelAlias();
                    if(!alias.empty()) output << "\t\talias: \"" << alias << "\",\n"; // alias

                    auto& items = stmt->getImportItems();
                    if(items.empty()) {
                        output << "\t\tsymbols : std::span<ImportSymbol>(),\n";
                    } else {
                        output << "\t\tsymbols: [ ";
                        for(auto& sym : items) {
                            output << "ImportSymbol { parts : ";
                            if(sym.parts.empty()) {
                                output << "std::span<std::string_view>(), ";
                            } else {
                                output << "[ ";
                                for (auto& part: sym.parts) {
                                    output << "std::string_view(\"" << part << "\"),";
                                }
                                output << "], ";
                            }
                            output << "alias: \"" << sym.alias << "\" }, ";
                        }
                        output << " ],\n";
                    }

                    output << "\t\tlocation: intrinsics::get_raw_location(),\n";
                    output << "\t});\n";
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