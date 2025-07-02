// Copyright (c) Chemical Language Foundation 2025.

#include "ASTDiagnoserCBI.h"
#include "compiler/ASTDiagnoser.h"

void ASTDiagnosererror(ASTDiagnoser* diagnoser, chem::string_view* msg, uint64_t loc) {
    diagnoser->error(*msg, loc);
}