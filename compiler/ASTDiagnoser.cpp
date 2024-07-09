// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "SelfInvocation.h"
#include <iostream>
#include "ast/base/ASTNode.h"
#include <filesystem>

#define ANSI_COLOR_RESET   "\x1b[0m"

void ASTDiagnoser::info(const std::string &err, ASTNode *node) {
    std::string errStr = "[INFO] " + err;
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Information);
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
    std::cout << "[" << TAG() << "] " << std::to_string(errors.size()) << " diagnostics in " << path << std::endl;
    for (const auto &err: errors) {
        std::cout << color(err.severity) << err.message << ANSI_COLOR_RESET << std::endl;
    }
}

void ASTDiagnoser::reset_errors() {
    has_errors = false;
    errors.clear();
}