// Copyright (c) Qinetik 2024.

#include "CSTDiagnoserCBI.h"
#include "cst/base/CSTDiagnoser.h"
#include "std/chem_string.h"

void CSTDiagnoserput_diagnostic(CSTDiagnoser* diagnoser, chem::string* msg, CSTToken* start, CSTToken* end, DiagSeverity severity) {
    auto view = std::string_view(msg->data(), msg->size());
    diagnoser->diagnostic(view, start, end, severity);
}