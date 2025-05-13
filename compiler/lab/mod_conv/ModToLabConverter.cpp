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
        output << stmt->as_identifier;
    } else {
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
    output << "func build(ctx : *mut BuildContext) : *Module {\n";
    output << "return ctx.create_module(\"" << data.scope_name << "\", \"" << data.module_name << "\", lab::rel_path_to(\"src\"), { ";
    // calling get functions on dependencies
    for(const auto node : data.scope.body.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ImportStmt: {
                const auto stmt = node->as_import_stmt_unsafe();
                writeAsIdentifier(stmt, output);
                output << ".get(), ";
                break;
            }
            default:
                continue;
        }
    }
    output << "}, { ";
    // writing each compiler interface
    for(const auto interface : data.compiler_interfaces) {
        output << '"' << interface << "\", ";
    }
    output << "});\n";
    output << "}\n";

    // get method
    output << "public func get(ctx : *mut BuildContext) : *Module {\n";
    output << "return ctx.default_get(\"" << data.scope_name << "\", \"" << data.module_name << "\", build);\n";
    output << "}\n";

}