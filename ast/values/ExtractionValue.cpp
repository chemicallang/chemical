// Copyright (c) Chemical Language Foundation 2025.

#include "ExtractionValue.h"
#include "ast/base/ASTAllocator.h"
#include "ast/types/PointerType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/VoidType.h"

BaseType* ExtractionValue::create_type(ASTAllocator &allocator) {
    switch(extractionKind) {
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