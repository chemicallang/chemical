// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "lexer/Lexer.h"
#include "stream/StreamSourceProvider.h"
#include "parser/Parser.h"
#include <filesystem>
#include <fstream>

class ImportStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ImportStatement object.
     *
     * @param filePath The file path to import.
     */
    ImportStatement(std::string filePath) : filePath(std::move(filePath)) {
        this->filePath.shrink_to_fit();
    }

    std::string representation() const override {
        return std::string("import \"" + filePath + "\";");
    }

    std::filesystem::path resolve_rel_path(InterpretScope& scope) {
        return (((std::filesystem::path) scope.global->root_path).parent_path() / ((std::filesystem::path) filePath));
    }

    void interpret(InterpretScope &scope) override {
        auto absolute_path = resolve_rel_path(scope).string();
        std::ifstream stream(absolute_path);
        if(stream.fail()) {
            scope.error("error couldn't import the following file " + absolute_path);
            std::cerr << "couldn't import the following file " + absolute_path;
            return;
        }
        StreamSourceProvider provider(stream);
        Lexer lexer(provider, absolute_path);
        lexer.lex();
        Parser parser(std::move(lexer.tokens));
        parser.parse();
        Scope fileScope(std::move(parser.nodes));
        auto prevPath = scope.global->root_path;
        scope.global->root_path = absolute_path;
        fileScope.interpret(scope);
        scope.global->root_path = prevPath;
    }

private:
    std::string filePath; ///< The file path to import.

};