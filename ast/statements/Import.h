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
    ImportStatement(std::string filePath, std::vector<std::string> identifiers);

    void accept(Visitor &visitor) override;

    std::string representation() const override;

    void code_gen(Codegen &gen) override;

    std::filesystem::path resolve_rel_path(InterpretScope& scope);

    void interpret(InterpretScope &scope);

private:
    std::vector<std::string> identifiers;
    std::string filePath; ///< The file path to import.

};