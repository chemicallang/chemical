// Copyright (c) Chemical Language Foundation 2025.

#include "ASTDiagnoser.h"
#include "ast/base/ASTNode.h"
#include "core/source/LocationManager.h"
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include <iostream>

void ASTDiagnoser::location_diagnostic(const chem::string_view& message, SourceLocation location, DiagSeverity severity) {
    const auto pos = loc_man.getLocationPos(location);
    const auto filePath = loc_man.getPathForFileId(pos.fileId);
    Diagnoser::diagnostic(chem::string_view(message), chem::string_view(filePath), pos.start, pos.end, severity);
}

Diag& ASTDiagnoser::empty_diagnostic(SourceLocation location, DiagSeverity severity) {
    const auto pos = loc_man.getLocationPos(location);
    const auto filePath = loc_man.getPathForFileId(pos.fileId);
    return Diagnoser::empty_diagnostic(chem::string_view(filePath), pos.start, pos.end, severity);
}

void ASTDiagnoser::print_debug_location(SourceLocation location) {
    auto& diag = empty_diagnostic(location, DiagSeverity::Hint);
    diag.format(std::cout, chem::string_view(diag.path_url.value()), "DEBUG_LOC");
}

void ASTDiagnoser::dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node) {
    error(new_node) << "duplicate symbol being declared, symbol '" << name << "' already exists";
    warn(previous) << "symbol has a conflict";
}

void ASTDiagnoser::unsatisfied_type_error(Value* value, BaseType* type) {
    const auto val_type = value->getType();
    if(val_type) {
        error(value) << "value with type '" << val_type->representation() << "' does not satisfy type '" << type->representation() << "'";
    } else {
        error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}