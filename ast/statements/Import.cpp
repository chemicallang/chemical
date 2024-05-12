// Copyright (c) Qinetik 2024.

#include "Import.h"
#include <filesystem>
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"

#define DEBUG false

#ifdef COMPILER_BUILD

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Tooling/Tooling.h"
#include "stream/StreamSourceProvider.h"
#include "utils/Utils.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Linker/Linker.h>

void ImportStatement::replace_at_in_path(ASTProcessor* processor) {
    if(filePath[0] != '@') return;
    auto slash = filePath.find('/');
    if(slash == -1) {
        processor->error("couldn't find '/' in the file path, which must be present if using '@' directive");
        return;
    }
    auto atDirective = filePath.substr(1, slash - 1);
    if(atDirective == "system") {
        auto headerPath = filePath.substr(slash + 1);
        // Resolve the containing directory to given header
        std::string dir = processor->headers_dir(headerPath);
        if (dir.empty()) {
            processor->error("couldn't resolve system headers directory for " + headerPath + " for importing");
            return;
        }
        // Absolute path to the header
        filePath = (std::filesystem::path(dir) / headerPath).string();
    } else {
        processor->error("unknown '@' directive " + atDirective + " in import statement");
    }
}

void ImportStatement::code_gen(Codegen &gen) {

    auto abs_path = resolve_rel_path(gen.current_path).string();
    auto found = gen.imported.find(abs_path);
    if(found == gen.imported.end()) {
        auto &ast = parsed(abs_path, [&abs_path, &gen](Diag* diag) {
            gen.error(diag->ansi_representation(abs_path, "Import"));
        }, gen.is64Bit, false, false, &gen);
        auto prev_path = gen.current_path;
        gen.current_path = abs_path;
        for(const auto &node : ast) {
            node->code_gen_declare(gen);
        }
        for (const auto &node: ast) {
//            std::cout << node->representation() << std::endl;
            node->code_gen(gen);
        }
        gen.current_path = prev_path;
        // clearing the cache to free memory
//        imported_ast.clear();
        gen.imported[abs_path] = true;
    }

}

#endif

namespace fs = std::filesystem;

std::vector<std::unique_ptr<ASTNode>> TranslateC(const char* exe_path, const char *abs_path, const char *resources_path);

std::vector<std::unique_ptr<ASTNode>>& ImportStatement::parsed(
        const std::string& resolved,
        std::function<void(Diag*)> handler,
        bool is64Bit,
        bool benchmark,
        bool print_representation,
        ASTProcessor* processor
) {

    if(!imported_ast.empty()) {
        return imported_ast;
    }

    if(resolved.ends_with(".h") || resolved.ends_with(".c")) {
        imported_ast = TranslateC(processor->curr_exe_path.c_str(), resolved.c_str(), processor->resources_dir.c_str());
        return imported_ast;
    }

    std::ifstream file;
    file.open(resolved);
    if (!file.is_open()) {
        std::cerr << "IMPORT STATEMENT FAILED with path : " + resolved << std::endl;
        return imported_ast;
    }
    auto lexer = benchmark ? benchLexFile(file, resolved) : lexFile(file, resolved);
    file.close();

    if(lexer.has_errors) {
        for(auto& err : lexer.errors) {
            handler(&err);
        }
    }

    CSTConverter converter(is64Bit);
    converter.convert(lexer.tokens);

    if(converter.has_errors) {
        for(auto& err : converter.diagnostics) {
            handler(&err);
        }
    }

    if(print_representation) {
        Scope scope(std::move(converter.nodes));
        std::cout << "[Representation]\n" << scope.representation() << std::endl;
        converter.nodes = std::move(scope.nodes);
    }

    imported_ast = std::move(converter.nodes);

    return imported_ast;

}


ImportStatement::ImportStatement(std::string filePath, std::vector<std::string> identifiers) : filePath(
        std::move(filePath)), identifiers(std::move(identifiers)) {
    this->filePath.shrink_to_fit();
}

void ImportStatement::declare_top_level(SymbolResolver &linker) {
    replace_at_in_path(&linker);
    auto abs_path = resolve_rel_path(linker.current_path).string();
    auto found = linker.imported.find(abs_path);
    if(found == linker.imported.end()) {
        linker.imported[abs_path] = true;
        auto &ast = parsed(abs_path, [&abs_path, &linker](Diag *diag) {
            linker.error(diag->ansi_representation(abs_path, "Import"));
        }, linker.is64Bit, linker.benchmark, linker.print_representation, &linker);
        auto previous = linker.current_path;
        linker.current_path = abs_path;
        for (const auto &node: ast) {
            node->declare_top_level(linker);
        }
        for (const auto &node: ast) {
            node->declare_and_link(linker);
        }
        linker.current_path = previous;
    }
}

void ImportStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string ImportStatement::representation() const {
    return std::string("import \"" + filePath + "\";\n");
}

std::filesystem::path ImportStatement::resolve_rel_path(const std::string& root_path) {
    return std::filesystem::canonical(((std::filesystem::path) root_path).parent_path() / ((std::filesystem::path) filePath));
}

void ImportStatement::interpret(InterpretScope &scope) {
    auto absolute_path = resolve_rel_path(scope.global->root_path).string();
    std::ifstream stream(absolute_path);
    if (stream.fail()) {
        scope.error("error couldn't import the following file " + absolute_path);
        return;
    }
    StreamSourceProvider provider(stream);
    Lexer lexer(provider, absolute_path);
    lexer.lex();
    // TODO convert to AST
//    Parser parser(std::move(lexer.tokens));
//    parser.parse();
//    Scope fileScope(std::move(parser.nodes));
//    auto prevPath = scope.global->root_path;
//    scope.global->root_path = absolute_path;
//    fileScope.interpret(scope);
//    scope.global->root_path = prevPath;
}