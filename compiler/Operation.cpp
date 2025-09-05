// Copyright (c) Chemical Language Foundation 2025.

#include "Codegen.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/IntNType.h"
#include "ast/values/IntNumValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"

#ifdef COMPILER_BUILD

#include "llvmimpl.h"
#include "ast/base/ASTNode.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/EnumDeclaration.h"

llvm::Value *Codegen::operate(Operation op, Value *lhs, Value *rhs) {
    auto firstType = lhs->getType();
    auto secondType = rhs->getType();
    return operate(op, lhs, rhs, firstType->pure_type(allocator), secondType->pure_type(allocator));
}

llvm::Value *Codegen::operate(Operation op, Value *lhs, Value *rhs, BaseType* lhsType, BaseType* rhsType) {
    return operate(op, lhs, rhs, lhsType, rhsType, lhs->llvm_value(*this), rhs->llvm_value(*this));
}

EnumDeclaration* getEnumDecl(BaseType* type, BaseTypeKind kind) {
    return kind == BaseTypeKind::Linked ? ((LinkedType*) type)->linked->as_enum_decl() : nullptr;
}

void perform_implicit_cast_on_integers(IntNType* fIntN, IntNType* secIntN, llvm::Value*& lhs, llvm::Value*& rhs, Codegen& gen) {
    // implicit cast int n types
    const auto lhsType = (llvm::IntegerType*) lhs->getType();
    const auto rhsType = (llvm::IntegerType*) rhs->getType();
    if(lhsType->getBitWidth() < rhsType->getBitWidth()) {
        if(fIntN->is_unsigned()) {
            lhs = gen.builder->CreateZExt(lhs, rhsType);
        } else {
            lhs = gen.builder->CreateSExt(lhs, rhsType);
        }
    } else if(lhsType->getBitWidth() > rhsType->getBitWidth()) {
        if(secIntN->is_unsigned()) {
            rhs = gen.builder->CreateZExt(rhs, lhsType);
        } else {
            rhs = gen.builder->CreateSExt(rhs, lhsType);
        }
    }
}

llvm::Value* cast_int_to_ptr(Codegen& gen, llvm::Value* value, llvm::Type* targetPtrTy) {
    auto& builder = gen.builder;
    const auto module = gen.module.get();
    auto expectedIntTy = builder->getIntPtrTy(module->getDataLayout(), targetPtrTy->getPointerAddressSpace());
    if (value->getType() != expectedIntTy) {
        value = builder->CreateIntCast(value, expectedIntTy, false); // unsigned cast is fine here
    }
    value = builder->CreateIntToPtr(value, targetPtrTy);
    return value;
}

llvm::Value *Codegen::operate(Operation op, Value *first, Value *second, BaseType* firstType, BaseType* secondType, llvm::Value* lhs, llvm::Value* rhs){

    // automatically dereference reference types
    if(firstType->kind() == BaseTypeKind::Reference) {
        const auto ref_type = firstType->as_reference_type_unsafe();
        const auto referred = ref_type->type->pure_type(allocator);
        const auto ref_kind = referred->kind();
        if(BaseType::isLoadableReferencee(ref_kind)) {
            const auto loadInst = builder->CreateLoad(referred->llvm_type(*this), lhs);
            di.instr(loadInst, first);
            lhs = loadInst;
            firstType = referred;
        }
    }
    if(secondType->kind() == BaseTypeKind::Reference) {
        const auto ref_type = secondType->as_reference_type_unsafe();
        const auto referred = ref_type->type->pure_type(allocator);
        const auto ref_kind = referred->kind();
        if(BaseType::isLoadableReferencee(ref_kind)) {
            const auto loadInst = builder->CreateLoad(referred->llvm_type(*this), rhs);
            di.instr(loadInst, second);
            rhs = loadInst;
            secondType = referred;
        }
    }

    // subtraction or addition to the pointer, pointer math
    if((op == Operation::Addition || op == Operation::Subtraction) && firstType->kind() == BaseTypeKind::Pointer) {
        const auto secType = rhs->getType();
        if(secType->isIntegerTy()) {
            llvm::Value *index = op == Operation::Addition ? rhs : builder->CreateNeg(rhs);
            return builder->CreateGEP(((PointerType *) firstType)->type->llvm_type(*this), lhs, {index}, "", inbounds);
        } else if(secType->isPointerTy()) {
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

    const auto firstTypeKind = firstType->kind();
    const auto secondTypeKind = secondType->kind();
    if(firstTypeKind == BaseTypeKind::IntN && secondTypeKind == BaseTypeKind::IntN) {
        perform_implicit_cast_on_integers((IntNType*) firstType, ((IntNType*) secondType), lhs, rhs, *this);
    } else {
        if(secondTypeKind == BaseTypeKind::IntN) {
            const auto firstEnum = getEnumDecl(firstType, firstTypeKind);
            if(firstEnum) {
                perform_implicit_cast_on_integers(firstEnum->get_underlying_integer_type(), ((IntNType*) secondType), lhs, rhs, *this);
            }
        } else if(firstTypeKind == BaseTypeKind::IntN) {
            const auto secondEnum = getEnumDecl(secondType, secondTypeKind);
            if(secondEnum) {
                perform_implicit_cast_on_integers(((IntNType*) firstType), secondEnum->get_underlying_integer_type(), lhs, rhs, *this);
            }
        }
    }

    // determine floatingness from the *LLVM* types after any casts/promotions
    const auto firstTypeFloating = lhs->getType()->isFloatingPointTy();
    const auto secondTypeFloating = rhs->getType()->isFloatingPointTy();
    const bool is_floating = firstTypeFloating || secondTypeFloating;

    // operation between integer and float values
    // promoting integer values to floats to perform the operation
    if(firstTypeKind == BaseTypeKind::IntN && secondTypeFloating) {
        const auto firstIntN = firstType->as_intn_type_unsafe();
        if(firstIntN->is_unsigned()) {
            lhs = builder->CreateUIToFP(lhs, secondType->llvm_type(*this));
        } else {
            lhs = builder->CreateSIToFP(lhs, secondType->llvm_type(*this));
        }
    } else if(secondTypeKind == BaseTypeKind::IntN && firstTypeFloating) {
        const auto secondIntN = secondType->as_intn_type_unsafe();
        if(secondIntN->is_unsigned()) {
            rhs = builder->CreateUIToFP(rhs, firstType->llvm_type(*this));
        } else {
            rhs = builder->CreateSIToFP(rhs, firstType->llvm_type(*this));
        }
    }

    const auto lhsType = lhs->getType();
    const auto rhsType = rhs->getType();

    // comparison of pointers with integers
    if (lhsType->isIntegerTy() && rhsType->isPointerTy()) {
        lhs = cast_int_to_ptr(*this, lhs, rhsType);
    } else if (lhsType->isPointerTy() && rhsType->isIntegerTy()) {
        rhs = cast_int_to_ptr(*this, rhs, lhsType);
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
            if(is_floating) {
                return builder->CreateFMul(lhs, rhs);
            } else {
                return builder->CreateMul(lhs, rhs);
            }
        case Operation::Division:
            if(is_floating) {
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
            if(is_floating) {
                return builder->CreateFAdd(lhs, rhs);
            } else {
                return builder->CreateAdd(lhs, rhs);
            }
        case Operation::Subtraction:
            if(is_floating) {
                return builder->CreateFSub(lhs, rhs);
            } else {
                return builder->CreateSub(lhs, rhs);
            }
        case Operation::LeftShift:
            return builder->CreateShl(lhs, rhs);
        case Operation::RightShift:
            if(is_unsigned()) {
                return builder->CreateLShr(lhs, rhs);
            } else {
                return builder->CreateAShr(lhs, rhs);
            }
        case Operation::GreaterThan:
            if(is_floating) {
                return builder->CreateFCmpOGT(lhs, rhs); // Floating greater than
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpUGT(lhs, rhs); // Unsigned greater than
                } else {
                    return builder->CreateICmpSGT(lhs, rhs); // Signed greater than
                }
            }
        case Operation::GreaterThanOrEqual:
            if(is_floating) {
                return builder->CreateFCmpOGE(lhs, rhs); // Floating greater than or equal
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpUGE(lhs, rhs); // Unsigned greater than or equal
                } else {
                    return builder->CreateICmpSGE(lhs, rhs); // Signed greater than or equal
                }
            }
        case Operation::LessThan:
            if(is_floating) {
                return builder->CreateFCmpOLT(lhs, rhs); // Floating less than
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpULT(lhs, rhs); // Unsigned less than
                } else {
                    return builder->CreateICmpSLT(lhs, rhs); // Signed less than
                }
            }
        case Operation::LessThanOrEqual:
            if(is_floating) {
                return builder->CreateFCmpOLE(lhs, rhs); // Floating less than or equal
            } else {
                if(is_unsigned()) {
                    return builder->CreateICmpULE(lhs, rhs); // Unsigned less than or equal
                } else {
                    return builder->CreateICmpSLE(lhs, rhs); // Signed less than or equal
                }
            }
        case Operation::IsEqual:
            if(is_floating) {
                return builder->CreateFCmpOEQ(lhs, rhs); // Floating Equal to
            } else {
                return builder->CreateICmpEQ(lhs, rhs); // Equal to
            }
        case Operation::IsNotEqual:
            if(is_floating) {
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
            error(first) << "Cannot operate on operation " << to_string(op);
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