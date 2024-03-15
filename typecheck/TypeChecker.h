// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <memory>
#include <vector>

class ASTNode;

class TypeChecker {
public:

    std::vector<std::unique_ptr<ASTNode>> nodes;

    std::vector<std::string> errors;

    TypeChecker(std::vector<std::unique_ptr<ASTNode>> nodes);

    void type_check();

    inline void error(const std::string& err) {
        errors.push_back(err);
    }

};