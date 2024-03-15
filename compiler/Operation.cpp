// Copyright (c) Qinetik 2024.

#include "Codegen.h"

#ifdef COMPILER_BUILD
llvm::Value* Codegen::operate(Operation op, llvm::Value* lhs, llvm::Value* rhs) {
    switch(op) {
//        case Operation::Grouping:
//            break;
//        case Operation::ScopeResolutionUnary:
//            break;
//        case Operation::ScopeResolutionBinary:
//            break;
//        case Operation::FunctionCall:
//            break;
//        case Operation::Subscript:
//            break;
//        case Operation::StructureMember:
//            break;
//        case Operation::StructurePointerMember:
//            break;
//        case Operation::PostfixIncrement:
//            break;
//        case Operation::PostfixDecrement:
//            break;
//        case Operation::LogicalNegate:
//            break;
//        case Operation::OnesComplement:
//            break;
//        case Operation::UnaryPlus:
//            break;
//        case Operation::UnaryMinus:
//            break;
//        case Operation::PrefixIncrement:
//            break;
//        case Operation::PrefixDecrement:
//            break;
//        case Operation::Indirection:
//            break;
//        case Operation::AddressOf:
//            break;
//        case Operation::Sizeof:
//            break;
//        case Operation::TypeConversion:
//            break;
        case Operation::Multiplication:
            return builder->CreateMul(lhs, rhs);
        case Operation::Division:
            return builder->CreateSDiv(lhs, rhs); // Signed division
        case Operation::Modulus:
            return builder->CreateSRem(lhs, rhs); // Signed remainder
        case Operation::Addition:
            return builder->CreateAdd(lhs, rhs);
        case Operation::Subtraction:
            return builder->CreateSub(lhs, rhs);
        case Operation::LeftShift:
            return builder->CreateShl(lhs, rhs);
        case Operation::RightShift:
            return builder->CreateLShr(lhs, rhs);
        case Operation::GreaterThan:
            return builder->CreateICmpSGT(lhs, rhs); // Signed greater than
        case Operation::GreaterThanOrEqual:
            return builder->CreateICmpSGE(lhs, rhs); // Signed greater than or equal
        case Operation::LessThan:
            return builder->CreateICmpSLT(lhs, rhs); // Signed less than
        case Operation::LessThanOrEqual:
            return builder->CreateICmpSLE(lhs, rhs); // Signed less than or equal
        case Operation::IsEqual:
            return builder->CreateICmpEQ(lhs, rhs); // Equal to
        case Operation::IsNotEqual:
            return builder->CreateICmpNE(lhs, rhs); // Not equal to
        case Operation::BitwiseAND:
            return builder->CreateAnd(lhs, rhs);
        case Operation::BitwiseXOR:
            return builder->CreateXor(lhs, rhs);
        case Operation::BitwiseOR:
            return builder->CreateOr(lhs, rhs);
        case Operation::LogicalAND:
            return builder->CreateAnd(lhs, rhs);
        case Operation::LogicalOR:
            return builder->CreateOr(lhs, rhs);
        default:
            error("Cannot operate on operation " + to_string(op));
            return nullptr;
//        case Operation::Conditional:
//            break;
//        case Operation::Assignment:
//            break;
//        case Operation::AddTo:
//            break;
//        case Operation::SubtractFrom:
//            break;
//        case Operation::MultiplyBy:
//            break;
//        case Operation::DivideBy:
//            break;
//        case Operation::ModuloBy:
//            break;
//        case Operation::ShiftLeftBy:
//            break;
//        case Operation::ShiftRightBy:
//            break;
//        case Operation::ANDWith:
//            break;
//        case Operation::ExclusiveORWith:
//            break;
//        case Operation::InclusiveORWith:
//            break;
    }
}
#endif