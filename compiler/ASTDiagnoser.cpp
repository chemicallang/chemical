// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "ast/base/ASTNode.h"

void ASTDiagnoser::diagnostic(std::string& err, ASTAny* node, DiagSeverity severity) {
    CSTDiagnoser::diagnostic(err, node->cst_token(), severity);
}

void ASTDiagnoser::diagnostic(std::string_view& err, ASTAny* node, DiagSeverity severity) {
    CSTDiagnoser::diagnostic(err, node->cst_token(), severity);
}