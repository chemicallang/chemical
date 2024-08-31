// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "ast/base/ASTNode.h"

void ASTDiagnoser::info(const std::string &err, ASTAny* node) {
    diagnostic(err, node->cst_token(), DiagSeverity::Information);
}

void ASTDiagnoser::warn(const std::string &err, ASTAny *node) {
    diagnostic(err, node->cst_token(), DiagSeverity::Warning);
}

void ASTDiagnoser::error(const std::string &err, ASTAny* node) {
    diagnostic(err, node->cst_token(), DiagSeverity::Error);
}