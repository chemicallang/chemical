// Copyright (c) Qinetik 2024.

#include "Import.h"
#include <filesystem>
#include "lexer/Lexi.h"
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/base/GlobalInterpretScope.h"

namespace fs = std::filesystem;

ImportStatement::ImportStatement(
        std::string filePath,
        std::vector<std::string> identifiers,
        ASTNode* parent_node,
        CSTToken* token
) : filePath(std::move(filePath)), identifiers(std::move(identifiers)), parent_node(parent_node), token(token) {
    this->filePath.shrink_to_fit();
}

void ImportStatement::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {

}

void ImportStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void ImportStatement::interpret(InterpretScope &scope) {
//    auto absolute_path = resolve_rel_parent_path_str(scope.global->root_path, filePath);
//    std::ifstream stream(absolute_path);
//    if (stream.fail()) {
//        scope.error("error couldn't import the following file " + absolute_path);
//        return;
//    }
//    SourceProvider provider(&stream);
//    Lexer lexer(provider, absolute_path);
//    lexer.init_complete();
//    lexer.lex();
    // TODO convert to AST
//    Parser parser(std::move(lexer.tokens));
//    parser.parse();
//    Scope fileScope(std::move(parser.nodes));
//    auto prevPath = scope.global->root_path;
//    scope.global->root_path = absolute_path;
//    fileScope.interpret(scope);
//    scope.global->root_path = prevPath;
}