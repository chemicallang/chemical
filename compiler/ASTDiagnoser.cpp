// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "ast/base/ASTNode.h"
#include "cst/LocationManager.h"

void ASTDiagnoser::location_diagnostic(const std::string_view& message, SourceLocation location, DiagSeverity severity) {
    const auto pos = loc_man.getLocationPos(location);
    const auto filePath = loc_man.getPathForFileId(pos.fileId);
    CSTDiagnoser::diagnostic(message, filePath, pos.start, pos.end, severity);
}