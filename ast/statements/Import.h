// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"

class ImportStatement : public ASTNode {
public:
    /**
     * @brief Construct a new ImportStatement object.
     *
     * @param filePath The file path to import.
     */
    ImportStatement(std::string filePath) : filePath(std::move(filePath)) {}

private:
    std::string filePath; ///< The file path to import.
};