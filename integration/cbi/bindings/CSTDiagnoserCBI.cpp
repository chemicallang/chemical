// Copyright (c) Qinetik 2024.

#include "CSTDiagnoserCBI.h"
#include "cst/base/CSTDiagnoser.h"
#include "std/chem_string.h"

void CSTDiagnoserput_diagnostic(CSTDiagnoser* diagnoser, chem::string* msg, chem::string* filePath, CSTToken* start, CSTToken* end, DiagSeverity severity) {
    diagnoser->diagnostic(msg->to_view(), filePath->to_view(), start, end, severity);
}