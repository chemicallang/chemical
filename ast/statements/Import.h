// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include <functional>
#include <filesystem>

class Diag;

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

    std::filesystem::path resolve_rel_path(const std::string& root_path);

    /**
     * this parses the imported file, if already parsed returns cached
     * @param path file path should be given
     */
    std::vector<std::unique_ptr<ASTNode>>& parsed(
            const std::string& path,
            std::function<void(Diag*)> handler,
            bool is64Bit,
            bool benchmark,
            bool print_representation
    );

    void declare_top_level(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope);

private:
    std::vector<std::string> identifiers;
    std::string filePath; ///< The file path to import.

    // these are cached parsed nodes that are filled when file is imported
    std::vector<std::unique_ptr<ASTNode>> imported_ast;

};