// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "ExpressionEvaluator.h"
#include "ast/values/IntValue.h"

// Definition of static member variable
std::unordered_map<int, std::function<std::unique_ptr<Value>(Value *, Value *)>> ExpressionEvaluator::functionVector;

constexpr int ExpressionEvaluator::compute(ValueType vt, ValueType vt2, Operation op) {
    return ((uint8_t) vt << 20) | ((uint8_t) vt2 << 10) | (uint8_t) op;
}

 constexpr int ExpressionEvaluator::computeIntToInt(Operation op) {
    return compute(ValueType::Int, ValueType::Int, op);
}

 constexpr int ExpressionEvaluator::computeIntToFloat(Operation op) {
    return compute(ValueType::Int, ValueType::Float, op);
}

void ExpressionEvaluator::prepareFunctions() {

    //    (int + int) -> int
    functionVector[computeIntToInt(Operation::Addition)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) + *static_cast<int *>(v2->get_value()));
    };

    //    (int - int) -> int
    functionVector[computeIntToInt(Operation::Subtraction)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) - *static_cast<int *>(v2->get_value()));
    };

    //    (int * int) -> int
    functionVector[computeIntToInt(Operation::Multiplication)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) * *static_cast<int *>(v2->get_value()));
    };

    //    (int / int) -> int
    functionVector[computeIntToInt(Operation::Division)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) / *static_cast<int *>(v2->get_value()));
    };

    //    (int % int) -> int
    functionVector[computeIntToInt(Operation::Modulus)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) % *static_cast<int *>(v2->get_value()));
    };

    //    (int == int) -> bool
    functionVector[computeIntToInt(Operation::Equal)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(v1->as_int() == v2->as_int());
    };

    //    (int != int) -> bool
    functionVector[computeIntToInt(Operation::NotEqual)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(
                *static_cast<int *>(v1->get_value()) != *static_cast<int *>(v2->get_value()));
    };

    //    (int < int) -> bool
    functionVector[computeIntToInt(Operation::LessThan)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(*static_cast<int *>(v1->get_value()) < *static_cast<int *>(v2->get_value()));
    };

    //    (int <= int) -> bool
    functionVector[computeIntToInt(Operation::LessThanOrEqual)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(
                *static_cast<int *>(v1->get_value()) <= *static_cast<int *>(v2->get_value()));
    };

    //    (int > int) -> bool
    functionVector[computeIntToInt(Operation::GreaterThan)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(*static_cast<int *>(v1->get_value()) > *static_cast<int *>(v2->get_value()));
    };

    //    (int >= int) -> bool
    functionVector[computeIntToInt(Operation::GreaterThanOrEqual)] = [&](Value *v1,
                                                                         Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<BoolValue>(
                *static_cast<int *>(v1->get_value()) >= *static_cast<int *>(v2->get_value()));
    };

    //    (int << int) -> int
    functionVector[computeIntToInt(Operation::LeftShift)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) << *static_cast<int *>(v2->get_value()));
    };

    //    (int >> int) -> int
    functionVector[computeIntToInt(Operation::RightShift)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) >> *static_cast<int *>(v2->get_value()));
    };

    //    (int ++) -> int
    functionVector[computeIntToInt(Operation::Increment)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>((*static_cast<int *>(v1->get_value()))++);
    };

    //    (int --) -> int
    functionVector[computeIntToInt(Operation::Decrement)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>((*static_cast<int *>(v1->get_value()))--);
    };

    //    (int & int) -> int
    functionVector[computeIntToInt(Operation::And)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) & *static_cast<int *>(v2->get_value()));
    };

    //    (int | int) -> int
    functionVector[computeIntToInt(Operation::Or)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) | *static_cast<int *>(v2->get_value()));
    };

    //    (int ^ int) -> int
    functionVector[computeIntToInt(Operation::Xor)] = [&](Value *v1, Value *v2) -> std::unique_ptr<Value> {
        return std::make_unique<IntValue>(*static_cast<int *>(v1->get_value()) ^ *static_cast<int *>(v2->get_value()));
    };

}