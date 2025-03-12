// Copyright (c) Chemical Language Foundation 2025.

#include "ASTDiagnoser.h"
#include "ast/base/ASTNode.h"
#include "cst/LocationManager.h"
#include <iostream>

void ASTDiagnoser::location_diagnostic(const chem::string_view& message, SourceLocation location, DiagSeverity severity) {
    const auto pos = loc_man.getLocationPos(location);
    const auto filePath = loc_man.getPathForFileId(pos.fileId);
    CSTDiagnoser::diagnostic(chem::string_view(message), chem::string_view(filePath), pos.start, pos.end, severity);
}

Diag& ASTDiagnoser::empty_diagnostic(SourceLocation location, DiagSeverity severity) {
    const auto pos = loc_man.getLocationPos(location);
    const auto filePath = loc_man.getPathForFileId(pos.fileId);
    return CSTDiagnoser::empty_diagnostic(chem::string_view(filePath), pos.start, pos.end, severity);
}

void ASTDiagnoser::print_debug_location(SourceLocation location) {
    auto& diag = empty_diagnostic(location, DiagSeverity::Hint);
    diag.format(std::cout, chem::string_view(diag.path_url.value()), "DEBUG_LOC");
}