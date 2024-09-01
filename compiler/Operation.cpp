// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/IntNType.h"
#include "ast/values/IntNumValue.h"
#include "ast/types/PointerType.h"

#ifdef COMPILER_BUILD

#include "llvmimpl.h"

llvm::Value *Codegen::operate(Operation op, Value *lhs, Value *rhs) {
    auto firstType = lhs->create_type();
    auto secondType = rhs->create_type();
    return operate(op, lhs, rhs, firstType->pure_type(), secondType->pure_type());
}

llvm::Value *Codegen::operate(Operation op, Value *first, Value *second, BaseType* firstType, BaseType* secondType){
    auto lhs = first->llvm_value(*this);
    auto rhs = second->llvm_value(*this);

    // subtraction or addition to the pointer, pointer math
    if((op == Operation::Addition || op == Operation::Subtraction) && firstType->kind() == BaseTypeKind::Pointer) {
        auto second_value_type = second->value_type();
        if(second_value_type >= ValueType::IntNStart && second_value_type <= ValueType::IntNEnd) {
            llvm::Value *index = op == Operation::Addition ? rhs : builder->CreateNeg(rhs);
            return builder->CreateGEP(((PointerType *) firstType)->type->llvm_type(*this), lhs, {index}, "", inbounds);
        } else if(second_value_type == ValueType::Pointer) {
            lhs = builder->CreatePointerCast(lhs, is64Bit ? llvm::Type::getInt64Ty(*ctx) : llvm::Type::getInt32Ty(*ctx));
            rhs = builder->CreatePointerCast(rhs, is64Bit ? llvm::Type::getInt64Ty(*ctx) : llvm::Type::getInt32Ty(*ctx));
            llvm::Value* final;
            if(op == Operation::Addition)  {
                final = builder->CreateAdd(lhs, rhs);
            } else {
                final = builder->CreateSub(lhs, rhs);
            }
            llvm::Value* sdivRhs;
            auto byteSize = ((PointerType*) firstType)->type->byte_size(is64Bit);
            if(is64Bit) {
                sdivRhs = builder->getInt64(byteSize);
            } else {
                sdivRhs = builder->getInt32(byteSize);
            }
            return builder->CreateSDiv(final, sdivRhs, "", true);
        }
    }

    // Mutating type to fit types
    if(lhs->getType() != rhs->getType()) {
        if(lhs->getType()->isArrayTy()) {
#ifdef DEBUG
            info("mutating type of array to " + secondType->representation() + " to perform operation", first);
#endif
            lhs->mutateType(rhs->getType());
        } else if(rhs->getType()->isArrayTy()) {
#ifdef DEBUG
            info("mutating type of array to " + firstType->representation() + " to perform operation", second);
#endif
            rhs->mutateType(lhs->getType());
        }
    }

    auto is_floating = [&firstType, &secondType] () -> bool {
        return firstType->satisfies(ValueType::Float) || firstType->satisfies(ValueType::Double) || secondType->satisfies(ValueType::Float) || secondType->satisfies(ValueType::Double);
    };
    auto is_unsigned = [&firstType, &secondType, this, first] () -> bool {
        auto firstKind = firstType->kind();
        auto secondKind = secondType->kind();
        if(firstKind == BaseTypeKind::Pointer && secondKind == BaseTypeKind::Pointer) {
            return true;
        }
        auto first_unsigned = firstKind == BaseTypeKind::IntN && ((IntNType*) firstType)->is_unsigned();
        auto second_unsigned = secondKind == BaseTypeKind::IntN && ((IntNType*) firstType)->is_unsigned();
        if((first_unsigned && !second_unsigned) || (!first_unsigned && second_unsigned)) {
            info("Operation between two IntN types, where one of them is unsigned and the other signed is error prone", first);
        }
        return first_unsigned || second_unsigned;
    };

    // implicit cast int n types
    if(firstType->kind() == BaseTypeKind::IntN && secondType->kind() == BaseTypeKind::IntN) {
        auto fIntN = (IntNType*) firstType;
        auto secIntN = (IntNType*) secondType;
        if(fIntN->num_bits() < secIntN->num_bits()) {
            if(fIntN->is_unsigned()) {
                lhs = builder->CreateZExt(lhs, secIntN->llvm_type(*this));
            } else {
                lhs = builder->CreateSExt(lhs, secIntN->llvm_type(*this));
            }
        } else if(fIntN->num_bits() > secIntN->num_bits()) {
            if(secIntN->is_unsigned()) {
                rhs = builder->CreateZExt(rhs, fIntN->llvm_type(*this));
            } else {
                rhs = builder->CreateSExt(rhs, fIntN->llvm_type(*this));
            }
        }
    }

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