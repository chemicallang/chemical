// Copyright (c) Chemical Language Foundation 2025.

#include "ExtractionValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/VoidType.h"
#include "ast/base/TypeBuilder.h"

BaseType* create_extraction_value_type(ASTAllocator &allocator, ExtractionKind kind) {
    switch(kind) {
        case ExtractionKind::LambdaFnPtr:
        case ExtractionKind::LambdaCapturedPtr:
        case ExtractionKind::LambdaCapturedDestructor:
            return new (allocator.allocate<PointerType>()) PointerType(
                    new (allocator.allocate<VoidType>()) VoidType(), true
            );
        case ExtractionKind::SizeOfLambdaCaptured:
        case ExtractionKind::AlignOfLambdaCaptured:
            return new (allocator.allocate<UBigIntType>()) UBigIntType();
    }
}

BaseType* create_extraction_value_type(TypeBuilder& builder, ExtractionKind kind) {
    switch(kind) {
        case ExtractionKind::LambdaFnPtr:
        case ExtractionKind::LambdaCapturedPtr:
        case ExtractionKind::LambdaCapturedDestructor:
            return builder.getPtrToVoid();
        case ExtractionKind::SizeOfLambdaCaptured:
        case ExtractionKind::AlignOfLambdaCaptured:
            return builder.getUBigIntType();
    }
}