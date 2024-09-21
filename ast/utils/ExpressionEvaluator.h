// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <unordered_map>
#include "ast/base/Value.h"
#include "Operation.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/IntValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StringValue.h"
#include "ast/base/InterpretScope.h"

namespace ExpressionEvaluators {

    typedef Value *(*EvaluatorFn)(InterpretScope& scope, Value *, Value *);

    static inline int index(ValueType vt, ValueType vt2, Operation op) {
        return ((uint8_t) vt << 20) | ((uint8_t) vt2 << 10) | (uint8_t) op;
    }

    constexpr int compute(ValueType vt, ValueType vt2, Operation op) {
        return ((uint8_t) vt << 20) | ((uint8_t) vt2 << 10) | (uint8_t) op;
    }

    constexpr int computeIntToInt(Operation op) {
        return compute(ValueType::Int, ValueType::Int, op);
    }

    constexpr int computeIntToFloat(Operation op) {
        return compute(ValueType::Int, ValueType::Float, op);
    }

    constexpr int computeFloatToInt(Operation op) {
        return compute(ValueType::Float, ValueType::Int, op);
    }

    constexpr int computeBoolToBool(Operation op) {
        return compute(ValueType::Bool, ValueType::Bool, op);
    }

    constexpr int computeCharToChar(Operation op) {
        return compute(ValueType::Char, ValueType::Char, op);
    }

    constexpr int computeStrToStr(Operation op) {
        return compute(ValueType::String, ValueType::String, op);
    }

    constexpr int computeFloatToFloat(Operation op) {
        return compute(ValueType::Float, ValueType::Float, op);
    }

    constexpr int computeDoubleToDouble(Operation op) {
        return compute(ValueType::Double, ValueType::Double, op);
    }

    const std::unordered_map<int, EvaluatorFn> ExpressionEvaluatorsMap = {

            {computeIntToInt(Operation::Addition),                 [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() + v2->as_int(), nullptr);
            }},

            //    (int - int) -> int
            {computeIntToInt(Operation::Subtraction),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() - v2->as_int(), nullptr);
            }},

            //    (int * int) -> int
            {computeIntToInt(Operation::Multiplication),           [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() * v2->as_int(), nullptr);
            }},

            //    (int / int) -> int
            {computeIntToInt(Operation::Division),                 [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() / v2->as_int(), nullptr);
            }},

            //    (int % int) -> int
            {computeIntToInt(Operation::Modulus),                  [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() % v2->as_int(), nullptr);
            }},

            //    (int == int) -> bool
            {computeIntToInt(Operation::IsEqual),                  [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() == v2->as_int(), nullptr);
            }},

            //    (int != int) -> bool
            {computeIntToInt(Operation::IsNotEqual),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() != v2->as_int(), nullptr);
            }},

            //    (int < int) -> bool
            {computeIntToInt(Operation::LessThan),                 [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() < v2->as_int(), nullptr);
            }},

            //    (int <= int) -> bool
            {computeIntToInt(Operation::LessThanOrEqual),          [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() <= v2->as_int(), nullptr);
            }},

            //    (int > int) -> bool
            {computeIntToInt(Operation::GreaterThan),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() > v2->as_int(), nullptr);
            }},

            //    (int >= int) -> bool
            {computeIntToInt(Operation::GreaterThanOrEqual),       [](InterpretScope& scope, Value *v1,
                                                                      Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() >= v2->as_int(), nullptr);
            }},

            //    (int << int) -> int
            {computeIntToInt(Operation::LeftShift),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() << v2->as_int(), nullptr);
            }},

            //    (int >> int) -> int
            {computeIntToInt(Operation::RightShift),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() >> v2->as_int(), nullptr);
            }},

            //    (int ++) -> int
            {computeIntToInt(Operation::PostfixIncrement),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() + 1, nullptr);
            }},

            //    (int --) -> int
            {computeIntToInt(Operation::PostfixDecrement),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() - 1, nullptr);
            }},

            //    (int & int) -> int
            {computeIntToInt(Operation::BitwiseAND),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() & v2->as_int(), nullptr);
            }},

            //    (int | int) -> int
            {computeIntToInt(Operation::BitwiseOR),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() | v2->as_int(), nullptr);
            }},

            //    (int ^ int) -> int
            {computeIntToInt(Operation::BitwiseXOR),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_int() ^ v2->as_int(), nullptr);
            }},

            // ---------------------------bool to bool ----------------------------

            //    (bool + bool) -> bool
            {computeBoolToBool(Operation::Addition),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() + v2->as_bool(), nullptr);
            }},

            //    (bool - bool) -> bool
            {computeBoolToBool(Operation::Subtraction),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() - v2->as_bool(), nullptr);
            }},

            //    (bool * bool) -> bool
            {computeBoolToBool(Operation::Multiplication),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() * v2->as_bool(), nullptr);
            }},

            //    (bool / bool) -> int
            {computeBoolToBool(Operation::Division),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_bool() / v2->as_bool(), nullptr);
            }},

            //    (bool % bool) -> int
            {computeBoolToBool(Operation::Modulus),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_bool() % v2->as_bool(), nullptr);
            }},

            //    (bool == bool) -> bool
            {computeBoolToBool(Operation::IsEqual),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() == v2->as_bool(), nullptr);
            }},

            //    (bool != bool) -> bool
            {computeBoolToBool(Operation::IsNotEqual),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() != v2->as_bool(), nullptr);
            }},

            //    (bool < bool) -> bool
            {computeBoolToBool(Operation::LessThan),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() < v2->as_bool(), nullptr);
            }},

            //    (bool <= bool) -> bool
            {computeBoolToBool(Operation::LessThanOrEqual),        [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() <= v2->as_bool(), nullptr);
            }},

            //    (bool > bool) -> bool
            {computeBoolToBool(Operation::GreaterThan),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() > v2->as_bool(), nullptr);
            }},

            //    (bool >= bool) -> bool
            {computeBoolToBool(Operation::GreaterThanOrEqual),     [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() >= v2->as_bool(), nullptr);
            }},

            //    (bool << bool) -> bool
            {computeBoolToBool(Operation::LeftShift),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() << v2->as_bool(), nullptr);
            }},

            //    (bool >> bool) -> bool
            {computeBoolToBool(Operation::RightShift),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() >> v2->as_bool(), nullptr);
            }},

            //    (bool & bool) -> bool
            {computeBoolToBool(Operation::BitwiseAND),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() & v2->as_bool(), nullptr);
            }},

            //    (bool | bool) -> bool
            {computeBoolToBool(Operation::BitwiseOR),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() | v2->as_bool(), nullptr);
            }},

            //    (bool ^ bool) -> bool
            {computeBoolToBool(Operation::BitwiseXOR),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_bool() ^ v2->as_bool(), nullptr);
            }},

            // ---------------------------char to char ----------------------------

            //    (char + char) -> int
            {computeCharToChar(Operation::Addition),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() + v2->as_char(), nullptr);
            }},

            //    (char - char) -> int
            {computeCharToChar(Operation::Subtraction),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() - v2->as_char(), nullptr);
            }},

            //    (char * char) -> int
            {computeCharToChar(Operation::Multiplication),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() * v2->as_char(), nullptr);
            }},

            //    (char / char) -> int
            {computeCharToChar(Operation::Division),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() / v2->as_char(), nullptr);
            }},

            //    (char % char) -> int
            {computeCharToChar(Operation::Modulus),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() % v2->as_char(), nullptr);
            }},

            //    (char == char) -> bool
            {computeCharToChar(Operation::IsEqual),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() == v2->as_char(), nullptr);
            }},

            //    (char != char) -> bool
            {computeCharToChar(Operation::IsNotEqual),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() != v2->as_char(), nullptr);
            }},

            //    (char < char) -> bool
            {computeCharToChar(Operation::LessThan),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() < v2->as_char(), nullptr);
            }},

            //    (char <= char) -> bool
            {computeCharToChar(Operation::LessThanOrEqual),        [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() <= v2->as_char(), nullptr);
            }},

            //    (char > char) -> bool
            {computeCharToChar(Operation::GreaterThan),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() > v2->as_char(), nullptr);
            }},

            //    (char >= char) -> bool
            {computeCharToChar(Operation::GreaterThanOrEqual),     [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_char() >= v2->as_char(), nullptr);
            }},

            //    (char << char) -> int
            {computeCharToChar(Operation::LeftShift),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() << v2->as_char(), nullptr);
            }},

            //    (char >> char) -> int
            {computeCharToChar(Operation::RightShift),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() >> v2->as_char(), nullptr);
            }},

            //    (char & char) -> int
            {computeCharToChar(Operation::BitwiseAND),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() & v2->as_char(), nullptr);
            }},

            //    (char | char) -> int
            {computeCharToChar(Operation::BitwiseOR),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() | v2->as_char(), nullptr);
            }},

            //    (char ^ char) -> int
            {computeCharToChar(Operation::BitwiseXOR),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<IntValue>()) IntValue(v1->as_char() ^ v2->as_char(), nullptr);
            }},

            // ---------------------------float to float ----------------------------


            //    (float + float) -> float
            {computeFloatToFloat(Operation::Addition),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() + v2->as_float(), nullptr);
            }},

            //    (float - float) -> int
            {computeFloatToFloat(Operation::Subtraction),          [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() - v2->as_float(), nullptr);
            }},

            //    (float * float) -> int
            {computeFloatToFloat(Operation::Multiplication),       [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() * v2->as_float(), nullptr);
            }},

            //    (float / float) -> int
            {computeFloatToFloat(Operation::Division),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() / v2->as_float(), nullptr);
            }},

            //    (float % float) -> int
//    functionVector[computeFloatToFloat(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() % v2->as_float());
//    }},

            //    (float == float) -> bool
            {computeFloatToFloat(Operation::IsEqual),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() == v2->as_float(), nullptr);
            }},

            //    (float != float) -> bool
            {computeFloatToFloat(Operation::IsNotEqual),           [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() != v2->as_float(), nullptr);
            }},

            //    (float < float) -> bool
            {computeFloatToFloat(Operation::LessThan),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() < v2->as_float(), nullptr);
            }},

            //    (float <= float) -> bool
            {computeFloatToFloat(Operation::LessThanOrEqual),      [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() <= v2->as_float(), nullptr);
            }},

            //    (float > float) -> bool
            {computeFloatToFloat(Operation::GreaterThan),          [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() > v2->as_float(), nullptr);
            }},

            //    (float >= float) -> bool
            {computeFloatToFloat(Operation::GreaterThanOrEqual),   [](InterpretScope& scope, Value *v1,
                                                                      Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_float() >= v2->as_float(), nullptr);
            }},

//    //    (float << float) -> int
//    functionVector[computeFloatToFloat(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() << v2->as_float());
//    }},
//
//    //    (float >> float) -> int
//    functionVector[computeFloatToFloat(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() >> v2->as_float());
//    }},
//
//    //    (float & float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() & v2->as_float());
//    }},
//
//    //    (float | float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() | v2->as_float());
//    }},
//
//    //    (float ^ float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_float() ^ v2->as_float());
//    }},


            // ---------------------------double to double ----------------------------

            //    (double + double) -> int
            {computeDoubleToDouble(Operation::Addition),           [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<DoubleValue>()) DoubleValue(v1->as_double() + v2->as_double(), nullptr);
            }},

            //    (double - double) -> int
            {computeDoubleToDouble(Operation::Subtraction),        [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<DoubleValue>()) DoubleValue(v1->as_double() - v2->as_double(), nullptr);
            }},

            //    (double * double) -> int
            {computeDoubleToDouble(Operation::Multiplication),     [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<DoubleValue>()) DoubleValue(v1->as_double() * v2->as_double(), nullptr);
            }},

            //    (double / double) -> int
            {computeDoubleToDouble(Operation::Division),           [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<DoubleValue>()) DoubleValue(v1->as_double() / v2->as_double(), nullptr);
            }},

            //    (double % double) -> int
//    functionVector[computeDoubleToDouble(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() % v2->as_double());
//    }},

            //    (double == double) -> bool
            {computeDoubleToDouble(Operation::IsEqual),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() == v2->as_double(), nullptr);
            }},

            //    (double != double) -> bool
            {computeDoubleToDouble(Operation::IsNotEqual),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() != v2->as_double(), nullptr);
            }},

            //    (double < double) -> bool
            {computeDoubleToDouble(Operation::LessThan),           [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() < v2->as_double(), nullptr);
            }},

            //    (double <= double) -> bool
            {computeDoubleToDouble(Operation::LessThanOrEqual),    [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() <= v2->as_double(), nullptr);
            }},

            //    (double > double) -> bool
            {computeDoubleToDouble(Operation::GreaterThan),        [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() > v2->as_double(), nullptr);
            }},

            //    (double >= double) -> bool
            {computeDoubleToDouble(Operation::GreaterThanOrEqual), [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_double() >= v2->as_double(), nullptr);
            }},

//    //    (double << double) -> int
//    functionVector[computeDoubleToDouble(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() << v2->as_double());
//    }},
//
//    //    (double >> double) -> int
//    functionVector[computeDoubleToDouble(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() >> v2->as_double());
//    }},
//
//    //    (double & double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() & v2->as_double());
//    }},
//
//    //    (double | double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() | v2->as_double());
//    }},
//
//    //    (double ^ double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_double() ^ v2->as_double());
//    }},

            // ---------------------------string to string ----------------------------

            //    (string + string) -> int
            {computeStrToStr(Operation::Addition),                 [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<StringValue>()) StringValue(v1->as_string() + v2->as_string(), nullptr);
            }},

            //    (string - string) -> int
//    functionVector[computeStrToStr(Operation::Subtraction)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() - v2->as_string());
//    }},

            //    (string * string) -> int
//    functionVector[computeStrToStr(Operation::Multiplication)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() * v2->as_string());
//    }},

            //    (string / string) -> int
//    functionVector[computeStrToStr(Operation::Division)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() / v2->as_string());
//    }},

            //    (string % string) -> int
//    functionVector[computeStrToStr(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() % v2->as_string());
//    }},

            //    (string == string) -> bool
            {computeStrToStr(Operation::IsEqual),                  [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() == v2->as_string(), nullptr);
            }},

            //    (string != string) -> bool
            {computeStrToStr(Operation::IsNotEqual),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() != v2->as_string(), nullptr);
            }},

            //    (string < string) -> bool
            {computeStrToStr(Operation::LessThan),                 [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() < v2->as_string(), nullptr);
            }},

            //    (string <= string) -> bool
            {computeStrToStr(Operation::LessThanOrEqual),          [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() <= v2->as_string(), nullptr);
            }},

            //    (string > string) -> bool
            {computeStrToStr(Operation::GreaterThan),              [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() > v2->as_string(), nullptr);
            }},

            //    (string >= string) -> bool
            {computeStrToStr(Operation::GreaterThanOrEqual),       [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_string() >= v2->as_string(), nullptr);
            }},

            //    (string << string) -> int
//    functionVector[computeStrToStr(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() << v2->as_string());
//    }},
//
//    //    (string >> string) -> int
//    functionVector[computeStrToStr(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() >> v2->as_string());
//    }},
//
//    //    (string & string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() & v2->as_string());
//    }},
//
//    //    (string | string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() | v2->as_string());
//    }},
//
//    //    (string ^ string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new (scope.allocate<IntValue>()) IntValue(v1->as_string() ^ v2->as_string());
//    }},

            // -------------------------------------------------------------------------
            // From here we start to work with different values
            // -------------------------------------------------------------------------


            //    (int + float) -> float
            {computeIntToFloat(Operation::Addition),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() + v2->as_float(), nullptr);
            }},

            //    (int - float) -> float
            {computeIntToFloat(Operation::Subtraction),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() - v2->as_float(), nullptr);
            }},

            //    (int * float) -> float
            {computeIntToFloat(Operation::Multiplication),         [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() * v2->as_float(), nullptr);
            }},

            //    (int / float) -> float
            {computeIntToFloat(Operation::Division),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() / v2->as_float(), nullptr);
            }},

            //    (int % float) -> int
//    functionVector[computeIntToInt(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() % v2->as_float());
//    }},

            //    (int == float) -> bool
            {computeIntToFloat(Operation::IsEqual),                [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() == v2->as_float(), nullptr);
            }},

            //    (int != float) -> bool
            {computeIntToFloat(Operation::IsNotEqual),             [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() != v2->as_float(), nullptr);
            }},

            //    (int < float) -> bool
            {computeIntToFloat(Operation::LessThan),               [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() < v2->as_float(), nullptr);
            }},

            //    (int <= float) -> bool
            {computeIntToFloat(Operation::LessThanOrEqual),        [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() <= v2->as_float(), nullptr);
            }},

            //    (int > float) -> bool
            {computeIntToFloat(Operation::GreaterThan),            [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() > v2->as_float(), nullptr);
            }},

            //    (int >= float) -> bool
            {computeIntToFloat(Operation::GreaterThanOrEqual),     [](InterpretScope& scope, Value *v1, Value *v2) -> Value * {
                return new (scope.allocate<BoolValue>()) BoolValue(v1->as_int() >= v2->as_float(), nullptr);
            }},

            //    (int << float) -> int
//    functionVector[computeIntToFloat(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() << v2->as_float());
//    }},

            //    (int >> float) -> int
//    functionVector[computeIntToFloat(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() >> v2->as_float());
//    }},

            //    (int & float) -> float
//    functionVector[computeIntToFloat(Operation::And)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() & v2->as_float());
//    }},

            //    (int | float) -> float
//    functionVector[computeIntToFloat(Operation::Or)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() | v2->as_float());
//    }},

            //    (int ^ float) -> float
//    functionVector[computeIntToFloat(Operation::Xor)] = [](Value *v1, Value *v2) -> Value* {
//        return new (scope.allocate<FloatValue>()) FloatValue(v1->as_int() ^ v2->as_float());
//    }},

    };

}