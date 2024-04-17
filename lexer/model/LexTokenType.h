// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <cstdint>

enum class LexTokenType : uint8_t {

    CharOperator,
    Operation,
    Char,
    Comment,
    MultilineComment,
    String,
    Bool,
    Macro,
    UserToken,

    // absolute strings
    Keyword,
    Number,
    Type,
    StringOperator,
    Variable,
    RawToken,

    // compound statements
    CompAssignment,
    CompBreak,
    CompContinue,
    CompIf,
    CompImport,
    CompIncDec,
    CompReturn,
    CompSwitch,
    CompTypealias,
    CompVarInit,
    CompBody,
    CompDoWhile,
    CompEnumDecl,
    CompForLoop,
    CompFunctionParam,
    CompFunction,
    CompStructDef,
    CompTryCatch,
    CompWhile,

    // compound types
    CompArrayType,
    CompFunctionType,
    CompGenericType,
    CompPointerType,
    CompAccessChain,

    // compound values
    CompArrayValue,
    CompCastValue,
    CompExpression,
    CompFunctionCall,
    CompIndexOp,
    CompLambda,
    CompNegative,
    CompNot,
    CompStructValue,

    // indexes
    IndexLastToken = CompStructValue,
    IndexAbsStrStart = Keyword,
    IndexAbsStrEnd = RawToken,
    IndexCompStart = CompAssignment,
    IndexCompEnd = CompStructValue,
    IndexCompTypeStart = CompArrayType,
    IndexCompTypeEnd = CompAccessChain,
    IndexCompValueStart = CompArrayValue,
    IndexCompValueEnd = CompStructValue

};