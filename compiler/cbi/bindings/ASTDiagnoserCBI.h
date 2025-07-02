// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>
#include "std/chem_string_view.h"

class ASTDiagnoser;

extern "C" {

    void ASTDiagnosererror(ASTDiagnoser* diagnoser, chem::string_view* msg, uint64_t loc);

}