// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "ExpressionEvaluator.h"
#include "ast/values/IntValue.h"

// Definition of static member variable
std::unordered_map<int, std::function<Value *(Value *, Value *)>> ExpressionEvaluator::functionVector;

constexpr int ExpressionEvaluator::compute(ValueType vt, ValueType vt2, Operation op) {
    return ((uint8_t) vt << 20) | ((uint8_t) vt2 << 10) | (uint8_t) op;
}

constexpr int ExpressionEvaluator::computeIntToInt(Operation op) {
    return compute(ValueType::Int, ValueType::Int, op);
}

constexpr int ExpressionEvaluator::computeIntToFloat(Operation op) {
    return compute(ValueType::Int, ValueType::Float, op);
}

constexpr int ExpressionEvaluator::computeFloatToInt(Operation op) {
    return compute(ValueType::Float, ValueType::Int, op);
}

constexpr int ExpressionEvaluator::computeBoolToBool(Operation op) {
    return compute(ValueType::Bool, ValueType::Bool, op);
}

constexpr int ExpressionEvaluator::computeCharToChar(Operation op) {
    return compute(ValueType::Char, ValueType::Char, op);
}

void ExpressionEvaluator::prepareFunctions() {

    //    (int + int) -> int
    functionVector[computeIntToInt(Operation::Addition)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() + v2->as_int());
    };

    //    (int - int) -> int
    functionVector[computeIntToInt(Operation::Subtraction)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() - v2->as_int());
    };

    //    (int * int) -> int
    functionVector[computeIntToInt(Operation::Multiplication)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() * v2->as_int());
    };

    //    (int / int) -> int
    functionVector[computeIntToInt(Operation::Division)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() / v2->as_int());
    };

    //    (int % int) -> int
    functionVector[computeIntToInt(Operation::Modulus)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() % v2->as_int());
    };

    //    (int == int) -> bool
    functionVector[computeIntToInt(Operation::IsEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() == v2->as_int());
    };

    //    (int != int) -> bool
    functionVector[computeIntToInt(Operation::IsNotEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() != v2->as_int());
    };

    //    (int < int) -> bool
    functionVector[computeIntToInt(Operation::LessThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() < v2->as_int());
    };

    //    (int <= int) -> bool
    functionVector[computeIntToInt(Operation::LessThanOrEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() <= v2->as_int());
    };

    //    (int > int) -> bool
    functionVector[computeIntToInt(Operation::GreaterThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() > v2->as_int());
    };

    //    (int >= int) -> bool
    functionVector[computeIntToInt(Operation::GreaterThanOrEqual)] = [&](Value *v1,
                                                                         Value *v2) -> Value * {
        return new BoolValue(v1->as_int() >= v2->as_int());
    };

    //    (int << int) -> int
    functionVector[computeIntToInt(Operation::LeftShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() << v2->as_int());
    };

    //    (int >> int) -> int
    functionVector[computeIntToInt(Operation::RightShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() >> v2->as_int());
    };

    //    (int ++) -> int
    functionVector[computeIntToInt(Operation::PostfixIncrement)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() + 1);
    };

    //    (int --) -> int
    functionVector[computeIntToInt(Operation::PostfixDecrement)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() - 1);
    };

    //    (int & int) -> int
    functionVector[computeIntToInt(Operation::BitwiseAND)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() & v2->as_int());
    };

    //    (int | int) -> int
    functionVector[computeIntToInt(Operation::BitwiseOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() | v2->as_int());
    };

    //    (int ^ int) -> int
    functionVector[computeIntToInt(Operation::BitwiseXOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_int() ^ v2->as_int());
    };

    //    (int + float) -> float
    functionVector[computeIntToFloat(Operation::Addition)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_int() + v2->as_float());
    };

    //    (int - float) -> float
    functionVector[computeIntToFloat(Operation::Subtraction)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_int() - v2->as_float());
    };

    //    (int * float) -> float
    functionVector[computeIntToFloat(Operation::Multiplication)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_int() * v2->as_float());
    };

    //    (int / float) -> float
    functionVector[computeIntToFloat(Operation::Division)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_int() / v2->as_float());
    };

    //    (int % float) -> int
//    functionVector[computeIntToInt(Operation::Modulus)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() % v2->as_float());
//    };

    //    (int == float) -> bool
    functionVector[computeIntToFloat(Operation::IsEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() == v2->as_float());
    };

    //    (int != float) -> bool
    functionVector[computeIntToFloat(Operation::IsNotEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() != v2->as_float());
    };

    //    (int < float) -> bool
    functionVector[computeIntToFloat(Operation::LessThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() < v2->as_float());
    };

    //    (int <= float) -> bool
    functionVector[computeIntToFloat(Operation::LessThanOrEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() <= v2->as_float());
    };

    //    (int > float) -> bool
    functionVector[computeIntToFloat(Operation::GreaterThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_int() > v2->as_float());
    };

    //    (int >= float) -> bool
    functionVector[computeIntToFloat(Operation::GreaterThanOrEqual)] = [&](Value *v1,
                                                                           Value *v2) -> Value * {
        return new BoolValue(v1->as_int() >= v2->as_float());
    };

    //    (int << float) -> int
//    functionVector[computeIntToFloat(Operation::LeftShift)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() << v2->as_float());
//    };

    //    (int >> float) -> int
//    functionVector[computeIntToFloat(Operation::RightShift)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() >> v2->as_float());
//    };

    //    (float ++) -> float
    functionVector[computeFloatToInt(Operation::PostfixIncrement)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_float() + 1);
    };

    //    (float --) -> float
    functionVector[computeFloatToInt(Operation::PostfixDecrement)] = [&](Value *v1, Value *v2) -> Value * {
        return new FloatValue(v1->as_float() - 1);
    };

    //    (int & float) -> float
//    functionVector[computeIntToFloat(Operation::And)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() & v2->as_float());
//    };

    //    (int | float) -> float
//    functionVector[computeIntToFloat(Operation::Or)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() | v2->as_float());
//    };

    //    (int ^ float) -> float
//    functionVector[computeIntToFloat(Operation::Xor)] = [&](Value *v1, Value *v2) -> Value* {
//        return new FloatValue(v1->as_int() ^ v2->as_float());
//    };

    //    (bool + bool) -> bool
    functionVector[computeBoolToBool(Operation::Addition)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() + v2->as_bool());
    };

    //    (bool - bool) -> bool
    functionVector[computeBoolToBool(Operation::Subtraction)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() - v2->as_bool());
    };

    //    (bool * bool) -> bool
    functionVector[computeBoolToBool(Operation::Multiplication)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() * v2->as_bool());
    };

    //    (bool / bool) -> int
    functionVector[computeBoolToBool(Operation::Division)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() / v2->as_bool());
    };

    //    (bool % bool) -> int
    functionVector[computeBoolToBool(Operation::Modulus)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() % v2->as_bool());
    };

    //    (bool == bool) -> bool
    functionVector[computeBoolToBool(Operation::IsEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() == v2->as_bool());
    };

    //    (bool != bool) -> bool
    functionVector[computeBoolToBool(Operation::IsNotEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() != v2->as_bool());
    };

    //    (bool < bool) -> bool
    functionVector[computeBoolToBool(Operation::LessThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() < v2->as_bool());
    };

    //    (bool <= bool) -> bool
    functionVector[computeBoolToBool(Operation::LessThanOrEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() <= v2->as_bool());
    };

    //    (bool > bool) -> bool
    functionVector[computeBoolToBool(Operation::GreaterThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() > v2->as_bool());
    };

    //    (bool >= bool) -> bool
    functionVector[computeBoolToBool(Operation::GreaterThanOrEqual)] = [&](Value *v1,
                                                                         Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() >= v2->as_bool());
    };

    //    (bool << bool) -> bool
    functionVector[computeBoolToBool(Operation::LeftShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() << v2->as_bool());
    };

    //    (bool >> bool) -> int
    functionVector[computeBoolToBool(Operation::RightShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() >> v2->as_bool());
    };

    //    (bool & bool) -> bool
    functionVector[computeBoolToBool(Operation::BitwiseAND)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() & v2->as_bool());
    };

    //    (bool | bool) -> bool
    functionVector[computeBoolToBool(Operation::BitwiseOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() | v2->as_bool());
    };

    //    (bool ^ bool) -> bool
    functionVector[computeBoolToBool(Operation::BitwiseXOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_bool() ^ v2->as_bool());
    };

    //    (char + char) -> int
    functionVector[computeCharToChar(Operation::Addition)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() + v2->as_char());
    };

    //    (char - char) -> int
    functionVector[computeCharToChar(Operation::Subtraction)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() - v2->as_char());
    };

    //    (char * char) -> int
    functionVector[computeCharToChar(Operation::Multiplication)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() * v2->as_char());
    };

    //    (char / char) -> int
    functionVector[computeCharToChar(Operation::Division)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() / v2->as_char());
    };

    //    (char % char) -> int
    functionVector[computeCharToChar(Operation::Modulus)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() % v2->as_char());
    };

    //    (char == char) -> bool
    functionVector[computeCharToChar(Operation::IsEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_char() == v2->as_char());
    };

    //    (char != char) -> bool
    functionVector[computeCharToChar(Operation::IsNotEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_char() != v2->as_char());
    };

    //    (char < char) -> bool
    functionVector[computeCharToChar(Operation::LessThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_char() < v2->as_char());
    };

    //    (char <= char) -> bool
    functionVector[computeCharToChar(Operation::LessThanOrEqual)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_char() <= v2->as_char());
    };

    //    (char > char) -> bool
    functionVector[computeCharToChar(Operation::GreaterThan)] = [&](Value *v1, Value *v2) -> Value * {
        return new BoolValue(v1->as_char() > v2->as_char());
    };

    //    (char >= char) -> bool
    functionVector[computeCharToChar(Operation::GreaterThanOrEqual)] = [&](Value *v1,
                                                                           Value *v2) -> Value * {
        return new BoolValue(v1->as_char() >= v2->as_char());
    };

    //    (char << char) -> int
    functionVector[computeCharToChar(Operation::LeftShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() << v2->as_char());
    };

    //    (char >> char) -> int
    functionVector[computeCharToChar(Operation::RightShift)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() >> v2->as_char());
    };

    //    (char & char) -> int
    functionVector[computeCharToChar(Operation::BitwiseAND)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() & v2->as_char());
    };

    //    (char | char) -> int
    functionVector[computeCharToChar(Operation::BitwiseOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() | v2->as_char());
    };

    //    (char ^ char) -> int
    functionVector[computeCharToChar(Operation::BitwiseXOR)] = [&](Value *v1, Value *v2) -> Value * {
        return new IntValue(v1->as_char() ^ v2->as_char());
    };

}