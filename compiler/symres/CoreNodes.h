// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ast/base/ast_fwd.h"
#include "ast/utils/Operation.h"

class CoreNodesOps {
public:

    FunctionDeclaration* add = nullptr;
    FunctionDeclaration* sub = nullptr;
    FunctionDeclaration* mul = nullptr;
    FunctionDeclaration* div = nullptr;
    FunctionDeclaration* rem = nullptr;
    FunctionDeclaration* neg = nullptr;
    FunctionDeclaration* _not = nullptr;

    // InterfaceDefinition* increment = nullptr;
    // InterfaceDefinition* decrement = nullptr;

    FunctionDeclaration* add_assign = nullptr;
    FunctionDeclaration* sub_assign = nullptr;
    FunctionDeclaration* mul_assign = nullptr;
    FunctionDeclaration* div_assign = nullptr;
    FunctionDeclaration* rem_assign = nullptr;

    FunctionDeclaration* bit_and_assign = nullptr;
    FunctionDeclaration* bit_or_assign = nullptr;
    FunctionDeclaration* bit_xor_assign = nullptr;
    FunctionDeclaration* shl_assign = nullptr;
    FunctionDeclaration* shr_assign = nullptr;

    FunctionDeclaration* bit_and = nullptr;
    FunctionDeclaration* bit_or = nullptr;
    FunctionDeclaration* bit_xor = nullptr;
    FunctionDeclaration* shl = nullptr;
    FunctionDeclaration* shr = nullptr;

    FunctionDeclaration* eq = nullptr;
    FunctionDeclaration* ne = nullptr;

    FunctionDeclaration* gt = nullptr;
    FunctionDeclaration* lt = nullptr;
    FunctionDeclaration* gte = nullptr;
    FunctionDeclaration* lte = nullptr;

    // InterfaceDefinition* partial_ord = nullptr;
    // InterfaceDefinition* ord = nullptr;

    FunctionDeclaration* index = nullptr;
    FunctionDeclaration* index_mut = nullptr;


};

class CoreNodes {
public:

    CoreNodesOps ops;

    /**
     * operator being overloaded, information about it is stored
     * in this struct
     */
    struct OperatorImplInformation {
        // the base interface that user should be overriding
        FunctionDeclaration* base;
        // the name of the function
        chem::string_view name;
    };

    /**
     * get information about operator being overloaded, like function name to use
     */
    OperatorImplInformation operator_impl_info(Operation op) {
        switch(op) {
            case Operation::Addition:
                return { .base = ops.add, .name = "add" };
            case Operation::Subtraction:
                return { .base = ops.sub, .name = "sub" };
            case Operation::Multiplication:
                return { .base = ops.mul, .name = "mul" };
            case Operation::Division:
                return { .base = ops.div, .name = "div" };
            case Operation::Modulus:
                return { .base = ops.rem, .name = "rem" };
            case Operation::BitwiseAND:
                return { .base = ops.bit_and, .name = "bitand" };
            case Operation::BitwiseOR:
                return { .base = ops.bit_or, .name = "bitor" };
            case Operation::BitwiseXOR:
                return { .base = ops.bit_xor, .name = "bitxor" };
            case Operation::LeftShift:
                return { .base = ops.shl, .name = "shl" };
            case Operation::RightShift:
                return { .base = ops.shr, .name = "shr" };
            case Operation::IsEqual:
                return { .base = ops.eq, .name = "eq" };
            case Operation::IsNotEqual:
                return { .base = ops.ne, .name = "ne" };
            case Operation::GreaterThan:
                return { .base = ops.gt, .name = "gt" };
            case Operation::LessThan:
                return { .base = ops.lt, .name = "lt" };
            case Operation::GreaterThanOrEqual:
                return { .base = ops.gte, .name = "gte" };
            case Operation::LessThanOrEqual:
                return { .base = ops.lte, .name = "lte" };
            default:
                return { .base = nullptr, .name = "" };
        }
    }

};