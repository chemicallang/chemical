// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "SelfInvocation.h"
#include <iostream>
#include "ast/base/ASTNode.h"
#include <filesystem>
#include "rang.hpp"

void ASTDiagnoser::info(const std::string &err, ASTNode *node) {
    std::string errStr = "[INFO] " + err;
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Information);
}

void ASTDiagnoser::warn(const std::string &err, ASTNode *node) {
    has_errors = true;
    std::string errStr = "[ERROR] " + err;
#ifdef DEBUG
    std::cerr << "[DEBUG:ERROR]" << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Warning);
}

void ASTDiagnoser::error(const std::string &err, ASTNode *node) {
    has_errors = true;
    std::string errStr = "[ERROR] " + err;
#ifdef DEBUG
    std::cerr << "[DEBUG:ERROR]" << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Error);
}

void ASTDiagnoser::print_errors(const std::string& path) {
    std::cout << rang::fg::cyan << "[" << TAG() << "] " << std::to_string(errors.size()) << " diagnostics in " << path << rang::fg::reset << std::endl;
    for (const auto &err: errors) {
        color(std::cout, err.severity) << err.message << rang::bg::reset << rang::fg::reset << std::endl;
    }
}

void ASTDiagnoser::reset_errors() {
    has_errors = false;
    errors.clear();
}