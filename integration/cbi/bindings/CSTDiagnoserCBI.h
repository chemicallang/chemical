// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"
#include "integration/common/DiagSeverity.h"

class CSTDiagnoser;

class Value;

class BaseType;

class ASTNode;

class CSTToken;

extern "C" {

    void CSTDiagnoserput_diagnostic(CSTDiagnoser* diagnoser, chem::string* msg, chem::string* filePath, CSTToken* start, CSTToken* end, DiagSeverity severity);

}