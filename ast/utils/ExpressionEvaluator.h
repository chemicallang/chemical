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

namespace ExpressionEvaluators {

    typedef Value *(*EvaluatorFn)(Value *, Value *);

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

            {computeIntToInt(Operation::Addition),                 [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() + v2->as_int(), nullptr);
            }},

            //    (int - int) -> int
            {computeIntToInt(Operation::Subtraction),              [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() - v2->as_int(), nullptr);
            }},

            //    (int * int) -> int
            {computeIntToInt(Operation::Multiplication),           [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() * v2->as_int(), nullptr);
            }},

            //    (int / int) -> int
            {computeIntToInt(Operation::Division),                 [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() / v2->as_int(), nullptr);
            }},

            //    (int % int) -> int
            {computeIntToInt(Operation::Modulus),                  [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() % v2->as_int(), nullptr);
            }},

            //    (int == int) -> bool
            {computeIntToInt(Operation::IsEqual),                  [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() == v2->as_int(), nullptr);
            }},

            //    (int != int) -> bool
            {computeIntToInt(Operation::IsNotEqual),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() != v2->as_int(), nullptr);
            }},

            //    (int < int) -> bool
            {computeIntToInt(Operation::LessThan),                 [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() < v2->as_int(), nullptr);
            }},

            //    (int <= int) -> bool
            {computeIntToInt(Operation::LessThanOrEqual),          [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() <= v2->as_int(), nullptr);
            }},

            //    (int > int) -> bool
            {computeIntToInt(Operation::GreaterThan),              [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() > v2->as_int(), nullptr);
            }},

            //    (int >= int) -> bool
            {computeIntToInt(Operation::GreaterThanOrEqual),       [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_int() >= v2->as_int(), nullptr);
            }},

            //    (int << int) -> int
            {computeIntToInt(Operation::LeftShift),                [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() << v2->as_int(), nullptr);
            }},

            //    (int >> int) -> int
            {computeIntToInt(Operation::RightShift),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() >> v2->as_int(), nullptr);
            }},

            //    (int ++) -> int
            {computeIntToInt(Operation::PostfixIncrement),         [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() + 1, nullptr);
            }},

            //    (int --) -> int
            {computeIntToInt(Operation::PostfixDecrement),         [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() - 1, nullptr);
            }},

            //    (int & int) -> int
            {computeIntToInt(Operation::BitwiseAND),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() & v2->as_int(), nullptr);
            }},

            //    (int | int) -> int
            {computeIntToInt(Operation::BitwiseOR),                [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() | v2->as_int(), nullptr);
            }},

            //    (int ^ int) -> int
            {computeIntToInt(Operation::BitwiseXOR),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_int() ^ v2->as_int(), nullptr);
            }},

            // ---------------------------bool to bool ----------------------------

            //    (bool + bool) -> bool
            {computeBoolToBool(Operation::Addition),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() + v2->as_bool(), nullptr);
            }},

            //    (bool - bool) -> bool
            {computeBoolToBool(Operation::Subtraction),            [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() - v2->as_bool(), nullptr);
            }},

            //    (bool * bool) -> bool
            {computeBoolToBool(Operation::Multiplication),         [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() * v2->as_bool(), nullptr);
            }},

            //    (bool / bool) -> int
            {computeBoolToBool(Operation::Division),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_bool() / v2->as_bool(), nullptr);
            }},

            //    (bool % bool) -> int
            {computeBoolToBool(Operation::Modulus),                [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_bool() % v2->as_bool(), nullptr);
            }},

            //    (bool == bool) -> bool
            {computeBoolToBool(Operation::IsEqual),                [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() == v2->as_bool(), nullptr);
            }},

            //    (bool != bool) -> bool
            {computeBoolToBool(Operation::IsNotEqual),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() != v2->as_bool(), nullptr);
            }},

            //    (bool < bool) -> bool
            {computeBoolToBool(Operation::LessThan),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() < v2->as_bool(), nullptr);
            }},

            //    (bool <= bool) -> bool
            {computeBoolToBool(Operation::LessThanOrEqual),        [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() <= v2->as_bool(), nullptr);
            }},

            //    (bool > bool) -> bool
            {computeBoolToBool(Operation::GreaterThan),            [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() > v2->as_bool(), nullptr);
            }},

            //    (bool >= bool) -> bool
            {computeBoolToBool(Operation::GreaterThanOrEqual),     [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() >= v2->as_bool(), nullptr);
            }},

            //    (bool << bool) -> bool
            {computeBoolToBool(Operation::LeftShift),              [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() << v2->as_bool(), nullptr);
            }},

            //    (bool >> bool) -> bool
            {computeBoolToBool(Operation::RightShift),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() >> v2->as_bool(), nullptr);
            }},

            //    (bool & bool) -> bool
            {computeBoolToBool(Operation::BitwiseAND),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() & v2->as_bool(), nullptr);
            }},

            //    (bool | bool) -> bool
            {computeBoolToBool(Operation::BitwiseOR),              [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() | v2->as_bool(), nullptr);
            }},

            //    (bool ^ bool) -> bool
            {computeBoolToBool(Operation::BitwiseXOR),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_bool() ^ v2->as_bool(), nullptr);
            }},

            // ---------------------------char to char ----------------------------

            //    (char + char) -> int
            {computeCharToChar(Operation::Addition),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() + v2->as_char(), nullptr);
            }},

            //    (char - char) -> int
            {computeCharToChar(Operation::Subtraction),            [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() - v2->as_char(), nullptr);
            }},

            //    (char * char) -> int
            {computeCharToChar(Operation::Multiplication),         [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() * v2->as_char(), nullptr);
            }},

            //    (char / char) -> int
            {computeCharToChar(Operation::Division),               [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() / v2->as_char(), nullptr);
            }},

            //    (char % char) -> int
            {computeCharToChar(Operation::Modulus),                [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() % v2->as_char(), nullptr);
            }},

            //    (char == char) -> bool
            {computeCharToChar(Operation::IsEqual),                [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_char() == v2->as_char(), nullptr);
            }},

            //    (char != char) -> bool
            {computeCharToChar(Operation::IsNotEqual),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_char() != v2->as_char(), nullptr);
            }},

            //    (char < char) -> bool
            {computeCharToChar(Operation::LessThan),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_char() < v2->as_char(), nullptr);
            }},

            //    (char <= char) -> bool
            {computeCharToChar(Operation::LessThanOrEqual),        [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_char() <= v2->as_char(), nullptr);
            }},

            //    (char > char) -> bool
            {computeCharToChar(Operation::GreaterThan),            [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_char() > v2->as_char(), nullptr);
            }},

            //    (char >= char) -> bool
            {computeCharToChar(Operation::GreaterThanOrEqual),     [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_char() >= v2->as_char(), nullptr);
            }},

            //    (char << char) -> int
            {computeCharToChar(Operation::LeftShift),              [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() << v2->as_char(), nullptr);
            }},

            //    (char >> char) -> int
            {computeCharToChar(Operation::RightShift),             [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() >> v2->as_char(), nullptr);
            }},

            //    (char & char) -> int
            {computeCharToChar(Operation::BitwiseAND),             [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() & v2->as_char(), nullptr);
            }},

            //    (char | char) -> int
            {computeCharToChar(Operation::BitwiseOR),              [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() | v2->as_char(), nullptr);
            }},

            //    (char ^ char) -> int
            {computeCharToChar(Operation::BitwiseXOR),             [](Value *v1, Value *v2) -> Value * {
                return new IntValue(v1->as_char() ^ v2->as_char(), nullptr);
            }},

            // ---------------------------float to float ----------------------------


            //    (float + float) -> float
            {computeFloatToFloat(Operation::Addition),             [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() + v2->as_float(), nullptr);
            }},

            //    (float - float) -> int
            {computeFloatToFloat(Operation::Subtraction),          [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() - v2->as_float(), nullptr);
            }},

            //    (float * float) -> int
            {computeFloatToFloat(Operation::Multiplication),       [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() * v2->as_float(), nullptr);
            }},

            //    (float / float) -> int
            {computeFloatToFloat(Operation::Division),             [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_float() / v2->as_float(), nullptr);
            }},

            //    (float % float) -> int
//    functionVector[computeFloatToFloat(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() % v2->as_float());
//    }},

            //    (float == float) -> bool
            {computeFloatToFloat(Operation::IsEqual),              [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_float() == v2->as_float(), nullptr);
            }},

            //    (float != float) -> bool
            {computeFloatToFloat(Operation::IsNotEqual),           [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_float() != v2->as_float(), nullptr);
            }},

            //    (float < float) -> bool
            {computeFloatToFloat(Operation::LessThan),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_float() < v2->as_float(), nullptr);
            }},

            //    (float <= float) -> bool
            {computeFloatToFloat(Operation::LessThanOrEqual),      [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_float() <= v2->as_float(), nullptr);
            }},

            //    (float > float) -> bool
            {computeFloatToFloat(Operation::GreaterThan),          [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_float() > v2->as_float(), nullptr);
            }},

            //    (float >= float) -> bool
            {computeFloatToFloat(Operation::GreaterThanOrEqual),   [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_float() >= v2->as_float(), nullptr);
            }},

//    //    (float << float) -> int
//    functionVector[computeFloatToFloat(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() << v2->as_float());
//    }},
//
//    //    (float >> float) -> int
//    functionVector[computeFloatToFloat(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() >> v2->as_float());
//    }},
//
//    //    (float & float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() & v2->as_float());
//    }},
//
//    //    (float | float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() | v2->as_float());
//    }},
//
//    //    (float ^ float) -> int
//    functionVector[computeFloatToFloat(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_float() ^ v2->as_float());
//    }},


            // ---------------------------double to double ----------------------------

            //    (double + double) -> int
            {computeDoubleToDouble(Operation::Addition),           [](Value *v1, Value *v2) -> Value * {
                return new DoubleValue(v1->as_double() + v2->as_double(), nullptr);
            }},

            //    (double - double) -> int
            {computeDoubleToDouble(Operation::Subtraction),        [](Value *v1, Value *v2) -> Value * {
                return new DoubleValue(v1->as_double() - v2->as_double(), nullptr);
            }},

            //    (double * double) -> int
            {computeDoubleToDouble(Operation::Multiplication),     [](Value *v1, Value *v2) -> Value * {
                return new DoubleValue(v1->as_double() * v2->as_double(), nullptr);
            }},

            //    (double / double) -> int
            {computeDoubleToDouble(Operation::Division),           [](Value *v1, Value *v2) -> Value * {
                return new DoubleValue(v1->as_double() / v2->as_double(), nullptr);
            }},

            //    (double % double) -> int
//    functionVector[computeDoubleToDouble(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() % v2->as_double());
//    }},

            //    (double == double) -> bool
            {computeDoubleToDouble(Operation::IsEqual),            [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_double() == v2->as_double(), nullptr);
            }},

            //    (double != double) -> bool
            {computeDoubleToDouble(Operation::IsNotEqual),         [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_double() != v2->as_double(), nullptr);
            }},

            //    (double < double) -> bool
            {computeDoubleToDouble(Operation::LessThan),           [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_double() < v2->as_double(), nullptr);
            }},

            //    (double <= double) -> bool
            {computeDoubleToDouble(Operation::LessThanOrEqual),    [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_double() <= v2->as_double(), nullptr);
            }},

            //    (double > double) -> bool
            {computeDoubleToDouble(Operation::GreaterThan),        [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_double() > v2->as_double(), nullptr);
            }},

            //    (double >= double) -> bool
            {computeDoubleToDouble(Operation::GreaterThanOrEqual), [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_double() >= v2->as_double(), nullptr);
            }},

//    //    (double << double) -> int
//    functionVector[computeDoubleToDouble(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() << v2->as_double());
//    }},
//
//    //    (double >> double) -> int
//    functionVector[computeDoubleToDouble(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() >> v2->as_double());
//    }},
//
//    //    (double & double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() & v2->as_double());
//    }},
//
//    //    (double | double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() | v2->as_double());
//    }},
//
//    //    (double ^ double) -> int
//    functionVector[computeDoubleToDouble(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_double() ^ v2->as_double());
//    }},

            // ---------------------------string to string ----------------------------

            //    (string + string) -> int
            {computeStrToStr(Operation::Addition),                 [](Value *v1, Value *v2) -> Value * {
                return new StringValue(v1->as_string() + v2->as_string(), nullptr);
            }},

            //    (string - string) -> int
//    functionVector[computeStrToStr(Operation::Subtraction)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() - v2->as_string());
//    }},

            //    (string * string) -> int
//    functionVector[computeStrToStr(Operation::Multiplication)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() * v2->as_string());
//    }},

            //    (string / string) -> int
//    functionVector[computeStrToStr(Operation::Division)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() / v2->as_string());
//    }},

            //    (string % string) -> int
//    functionVector[computeStrToStr(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() % v2->as_string());
//    }},

            //    (string == string) -> bool
            {computeStrToStr(Operation::IsEqual),                  [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_string() == v2->as_string(), nullptr);
            }},

            //    (string != string) -> bool
            {computeStrToStr(Operation::IsNotEqual),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_string() != v2->as_string(), nullptr);
            }},

            //    (string < string) -> bool
            {computeStrToStr(Operation::LessThan),                 [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_string() < v2->as_string(), nullptr);
            }},

            //    (string <= string) -> bool
            {computeStrToStr(Operation::LessThanOrEqual),          [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_string() <= v2->as_string(), nullptr);
            }},

            //    (string > string) -> bool
            {computeStrToStr(Operation::GreaterThan),              [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_string() > v2->as_string(), nullptr);
            }},

            //    (string >= string) -> bool
            {computeStrToStr(Operation::GreaterThanOrEqual),       [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_string() >= v2->as_string(), nullptr);
            }},

            //    (string << string) -> int
//    functionVector[computeStrToStr(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() << v2->as_string());
//    }},
//
//    //    (string >> string) -> int
//    functionVector[computeStrToStr(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() >> v2->as_string());
//    }},
//
//    //    (string & string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseAND)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() & v2->as_string());
//    }},
//
//    //    (string | string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() | v2->as_string());
//    }},
//
//    //    (string ^ string) -> int
//    functionVector[computeStrToStr(Operation::BitwiseXOR)] = [](Value *v1, Value *v2) -> Value * {
//        return new IntValue(v1->as_string() ^ v2->as_string());
//    }},

            // -------------------------------------------------------------------------
            // From here we start to work with different values
            // -------------------------------------------------------------------------


            //    (int + float) -> float
            {computeIntToFloat(Operation::Addition),               [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_int() + v2->as_float(), nullptr);
            }},

            //    (int - float) -> float
            {computeIntToFloat(Operation::Subtraction),            [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_int() - v2->as_float(), nullptr);
            }},

            //    (int * float) -> float
            {computeIntToFloat(Operation::Multiplication),         [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_int() * v2->as_float(), nullptr);
            }},

            //    (int / float) -> float
            {computeIntToFloat(Operation::Division),               [](Value *v1, Value *v2) -> Value * {
                return new FloatValue(v1->as_int() / v2->as_float(), nullptr);
            }},

            //    (int % float) -> int
//    functionVector[computeIntToInt(Operation::Modulus)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() % v2->as_float());
//    }},

            //    (int == float) -> bool
            {computeIntToFloat(Operation::IsEqual),                [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() == v2->as_float(), nullptr);
            }},

            //    (int != float) -> bool
            {computeIntToFloat(Operation::IsNotEqual),             [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() != v2->as_float(), nullptr);
            }},

            //    (int < float) -> bool
            {computeIntToFloat(Operation::LessThan),               [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() < v2->as_float(), nullptr);
            }},

            //    (int <= float) -> bool
            {computeIntToFloat(Operation::LessThanOrEqual),        [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() <= v2->as_float(), nullptr);
            }},

            //    (int > float) -> bool
            {computeIntToFloat(Operation::GreaterThan),            [](Value *v1, Value *v2) -> Value * {
                return new BoolValue(v1->as_int() > v2->as_float(), nullptr);
            }},

            //    (int >= float) -> bool
            {computeIntToFloat(Operation::GreaterThanOrEqual),     [](Value *v1,
                                                                      Value *v2) -> Value * {
                return new BoolValue(v1->as_int() >= v2->as_float(), nullptr);
            }},

            //    (int << float) -> int
//    functionVector[computeIntToFloat(Operation::LeftShift)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() << v2->as_float());
//    }},

            //    (int >> float) -> int
//    functionVector[computeIntToFloat(Operation::RightShift)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() >> v2->as_float());
//    }},

            //    (int & float) -> float
//    functionVector[computeIntToFloat(Operation::And)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() & v2->as_float());
//    }},

            //    (int | float) -> float
//    functionVector[computeIntToFloat(Operation::Or)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() | v2->as_float());
//    }},

            //    (int ^ float) -> float
//    functionVector[computeIntToFloat(Operation::Xor)] = [](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() ^ v2->as_float());
//    }},

    };

}