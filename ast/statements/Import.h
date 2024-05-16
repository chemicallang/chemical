// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "utils/fwd/functional.h"

class Diag;

class ASTProcessor;

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

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void declare_top_level(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope);

    std::vector<std::string> identifiers;
    std::string filePath; ///< The file path to import.

};