// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/IntNType.h"

#ifdef COMPILER_BUILD

#include "llvmimpl.h"

llvm::Value *Codegen::operate(Operation op, Value *first, Value *second) {
    auto firstType = first->create_type();
    auto secondType = second->create_type();
    auto lhs = first->llvm_value(*this);
    auto rhs = second->llvm_value(*this);

    // Mutating type to fit types
    if(lhs->getType() != rhs->getType()) {
        if(lhs->getType()->isArrayTy()) {
#ifdef DEBUG
            info("mutating type of array to " + secondType->representation() + " to perform operation");
#endif
            lhs->mutateType(rhs->getType());
        } else if(rhs->getType()->isArrayTy()) {
#ifdef DEBUG
            info("mutating type of array to " + firstType->representation() + " to perform operation");
#endif
            rhs->mutateType(lhs->getType());
        }
    }

    auto is_floating = [&firstType, &secondType] () -> bool {
        return firstType->satisfies(ValueType::Float) || firstType->satisfies(ValueType::Double) || secondType->satisfies(ValueType::Float) || secondType->satisfies(ValueType::Double);
    };
    auto is_unsigned = [&firstType, &secondType, this] () -> bool {
        auto first_unsigned = firstType->kind() == BaseTypeKind::IntN && ((IntNType*) (firstType.get()))->is_unsigned;
        auto second_unsigned = secondType->kind() == BaseTypeKind::IntN && ((IntNType*) (firstType.get()))->is_unsigned;
        if((first_unsigned && !second_unsigned) || (!first_unsigned && second_unsigned)) {
            this->error("Operation between two IntN types, where one of them is unsigned and the other signed is error prone");
        }
        return first_unsigned || second_unsigned;
    };
    switch (op) {
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
            if(is_floating()) {
                return builder->CreateFMul(lhs, rhs);
            } else {
                return builder->CreateMul(lhs, rhs);
            }
        case Operation::Division:
            if(is_floating()) {
                return builder->CreateFDiv(lhs, rhs);
            } else {
                if(is_unsigned()) {
                    return builder->CreateUDiv(lhs, rhs);
                } else {
                    return builder->CreateSDiv(lhs, rhs);
                }
            }
        case Operation::Modulus:
            return builder->CreateSRem(lhs, rhs); // Signed remainder
        case Operation::Addition:
            if(is_floating()) {
                return builder->CreateFAdd(lhs, rhs);
            } else {
                return builder->CreateAdd(lhs, rhs);
            }
        case Operation::Subtraction:
            if(is_floating()) {
                return builder->CreateFSub(lhs, rhs);
            } else {
                return builder->CreateSub(lhs, rhs);
            }
        case Operation::LeftShift:
            return builder->CreateShl(lhs, rhs);
        case Operation::RightShift:
            return builder->CreateLShr(lhs, rhs);
        case Operation::GreaterThan:
            if(is_floating()) {
                return builder->CreateFCmpOGT(lhs, rhs); // Floating greater than
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpUGT(lhs, rhs); // Unsigned greater than
                } else {
                    return builder->CreateICmpSGT(lhs, rhs); // Signed greater than
                }
            }
        case Operation::GreaterThanOrEqual:
            if(is_floating()) {
                return builder->CreateFCmpOGE(lhs, rhs); // Floating greater than or equal
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpUGE(lhs, rhs); // Unsigned greater than or equal
                } else {
                    return builder->CreateICmpSGE(lhs, rhs); // Signed greater than or equal
                }
            }
        case Operation::LessThan:
            if(is_floating()) {
                return builder->CreateFCmpOLT(lhs, rhs); // Floating less than
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpULT(lhs, rhs); // Unsigned less than
                } else {
                    return builder->CreateICmpSLT(lhs, rhs); // Signed less than
                }
            }
        case Operation::LessThanOrEqual:
            if(is_floating()) {
                return builder->CreateFCmpOLE(lhs, rhs); // Floating less than or equal
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpULE(lhs, rhs); // Unsigned less than or equal
                } else {
                    return builder->CreateICmpSLE(lhs, rhs); // Signed less than or equal
                }
            }
        case Operation::IsEqual:
            if(is_floating()) {
                return builder->CreateFCmpOEQ(lhs, rhs); // Floating Equal to
            } else {
                return builder->CreateICmpEQ(lhs, rhs); // Equal to
            }
        case Operation::IsNotEqual:
            if(is_floating()) {
                return builder->CreateFCmpONE(lhs, rhs); // Floating Not equal to
            } else {
                return builder->CreateICmpNE(lhs, rhs); // Not equal to
            }
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