// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <cstdint>
#include <string>

enum class LexTokenType : uint8_t {

    CharOperator,
    Operation,
    Char,
    Comment,
    MultilineComment,
    String,
    Bool,
    Annotation,
    UserToken,

    Keyword,
    Number,
    Type,
    Null,
    StringOperator,
    Variable,
    Identifier,
    RawToken,

    // compound statements
    CompAssignment,
    CompAccessChainNode,
    CompValueNode,
    CompAnnotation,
    CompBreak,
    CompContinue,
    CompInitBlock,
    CompThrow,
    CompUsing,
    CompDestruct,
    CompIf,
    CompImport,
    CompIncDec,
    CompReturn,
    CompSwitch,
    CompTypealias,
    CompVarInit,
    CompStructMember,
    CompBody,
    CompLoopBlock,
    CompDoWhile,
    CompEnumDecl,
    CompForLoop,
    CompFunctionParam,
    CompFunction,
    CompGenericParamsList,
    CompStructDef,
    CompVariant,
    CompVariantMember,
    CompUnionDef,
    CompInterface,
    CompImpl,
    CompNamespace,
    CompTryCatch,
    CompMacro,
    CompWhile,

    // compound types
    CompArrayType, // an array type is like int[]
    CompFunctionType, // a function type is like (a : int) => void
    CompGenericType, // generic type like my_gen<int, long>
    CompSpecializedType, // 'dyn type' is a specialized type, we could support more specialized types like dyn
    CompLinkedValueType, // linked value type is like (ns::class ns::class::class) basically anything with ::
    CompReferenceType, // a reference '&'
    CompPointerType, // a pointer '*'

    // compound values
    CompAccessChain, // a value representing a.b.c <--- an access chain
    CompArrayValue, // array value -> { 10, 20, 30 }
    CompIfValue, // if value is 'var a = if(condition) value else value2' <--- if value
    CompLoopValue, // similar to if value
    CompSwitchValue, // similar to if value
    CompCastValue, // casted value '3 as int'
    CompIsValue, // is value '3 is int' it returns boolean
    CompAddrOf, // address of value '&my_variable'
    CompDeference, // dereference value '*my_variable'
    CompExpression, // expression 'a + b'
    CompFunctionCall,
    CompIndexOp, // index operator 'array[2]'
    CompLambda, // lambda is the actual lambda []() => {  }
    CompNegative, // -value
    CompNot, // !value
    CompStructValue, // struct_name { a : 10, b : 20 }

    // other things
    CompGenericList,

    // indexes
    IndexLastToken = CompGenericList,
    IndexAbsStrStart = Keyword,
    IndexAbsStrEnd = RawToken,
    IndexCompStart = CompAssignment,
    IndexCompEnd = CompGenericList,
    IndexCompTypeStart = CompArrayType,
    IndexCompTypeEnd = CompPointerType,
    IndexCompValueStart = CompAccessChain,
    IndexCompValueEnd = CompStructValue

};

std::string toTypeString(LexTokenType token);