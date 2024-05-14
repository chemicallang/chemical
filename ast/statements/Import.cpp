// Copyright (c) Qinetik 2024.

#include "Import.h"
#include <filesystem>
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "stream/SourceProvider.h"

void ImportStatement::code_gen(Codegen &gen) {

}

#endif

namespace fs = std::filesystem;

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

ImportStatement::ImportStatement(std::string filePath, std::vector<std::string> identifiers) : filePath(
        std::move(filePath)), identifiers(std::move(identifiers)) {
    this->filePath.shrink_to_fit();
}

void ImportStatement::declare_top_level(SymbolResolver &linker) {

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
    SourceProvider provider(stream);
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