// Copyright (c) Chemical Language Foundation 2026.

#pragma once

class ASTDiagnoser;

class Value;

class ModuleScope;

/**
 * returns true on valid, false on invalid
 */
bool CheckTopLevelComptimeIfStmtCondition(
    ASTDiagnoser& diagnoser,
    Value* condition,
    ModuleScope* currModScope
);