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
    FunctionDeclaration* bitnot = nullptr;

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

    FunctionDeclaration* inc_pre = nullptr;
    FunctionDeclaration* inc_post = nullptr;
    FunctionDeclaration* dec_pre = nullptr;
    FunctionDeclaration* dec_post = nullptr;

    FunctionDeclaration* index = nullptr;
    FunctionDeclaration* index_mut = nullptr;


};

class CoreNodesIterable {
public:

    FunctionDeclaration* linear_data = nullptr;

    FunctionDeclaration* linear_size = nullptr;

    FunctionDeclaration* chunked_begin_chunks = nullptr;

    FunctionDeclaration* chunked_valid_chunk = nullptr;

    FunctionDeclaration* chunked_current_chunk = nullptr;

    FunctionDeclaration* chunked_next_chunk = nullptr;

    FunctionDeclaration* chunked_rbegin_chunks = nullptr;

    FunctionDeclaration* chunked_previous_chunk = nullptr;

    FunctionDeclaration* chunked_total_size = nullptr;

    FunctionDeclaration* iterable_begin = nullptr;

    FunctionDeclaration* iterable_valid = nullptr;

    FunctionDeclaration* iterable_current = nullptr;

    FunctionDeclaration* iterable_next = nullptr;

    FunctionDeclaration* reversible_iterable_rbegin = nullptr;

    FunctionDeclaration* reversible_iterable_previous = nullptr;

    FunctionDeclaration* reversible_iterable_count = nullptr;

};

class CoreNodesStream {
public:

    FunctionDeclaration* stream_write_signed = nullptr;
    FunctionDeclaration* stream_write_unsigned = nullptr;
    FunctionDeclaration* stream_write_str = nullptr;
    FunctionDeclaration* stream_write_str_no_len = nullptr;
    FunctionDeclaration* stream_write_float = nullptr;
    FunctionDeclaration* stream_write_double = nullptr;
    FunctionDeclaration* stream_write_char = nullptr;
    FunctionDeclaration* stream_write_uchar = nullptr;

};

class CoreNodes {
public:

    CoreNodesOps ops;

    CoreNodesIterable iterable;

    CoreNodesStream stream;

    /**
     * get information about operator being overloaded, like function name to use
     */
    FunctionDeclaration* expr_operator_impl_base(Operation op) {
        switch(op) {
            case Operation::Addition:
                return ops.add;
            case Operation::Subtraction:
                return ops.sub;
            case Operation::Multiplication:
                return ops.mul;
            case Operation::Division:
                return ops.div;
            case Operation::Modulus:
                return ops.rem;
            case Operation::BitwiseAND:
                return ops.bit_and;
            case Operation::BitwiseOR:
                return ops.bit_or;
            case Operation::BitwiseXOR:
                return ops.bit_xor;
            case Operation::LeftShift:
                return ops.shl;
            case Operation::RightShift:
                return ops.shr;
            case Operation::IsEqual:
                return ops.eq;
            case Operation::IsNotEqual:
                return ops.ne;
            case Operation::GreaterThan:
                return ops.gt;
            case Operation::LessThan:
                return ops.lt;
            case Operation::GreaterThanOrEqual:
                return ops.gte;
            case Operation::LessThanOrEqual:
                return ops.lte;
            default:
                return nullptr;
        }
    }

    FunctionDeclaration* assignment_operator_impl_base(Operation op) {
        switch(op) {
            case Operation::Addition:
                return ops.add_assign;
            case Operation::Subtraction:
                return ops.sub_assign;
            case Operation::Multiplication:
                return ops.mul_assign;
            case Operation::Division:
                return ops.div_assign;
            case Operation::Modulus:
                return ops.rem_assign;
            case Operation::BitwiseAND:
                return ops.bit_and_assign;
            case Operation::BitwiseOR:
                return ops.bit_or_assign;
            case Operation::BitwiseXOR:
                return ops.bit_xor_assign;
            case Operation::LeftShift:
                return ops.shl_assign;
            case Operation::RightShift:
                return ops.shr_assign;
            default:
                return nullptr;
        }
    }

    FunctionDeclaration* inc_dec_operator_impl_base(bool increment, bool post) {
        return increment ? (post ? ops.inc_post : ops.inc_pre) : (post ? ops.dec_post : ops.dec_pre);
    }

    void clear() {
        *this = CoreNodes {};
    }

};
