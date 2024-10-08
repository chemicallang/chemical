// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "CTranslator.h"
#include "ast/types/IntType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/LongType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/BoolType.h"
#include "ast/types/UShortType.h"
#include "ast/types/UIntType.h"
#include "ast/types/ULongType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/ShortType.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/Int128Type.h"

CTranslator::CTranslator(ASTAllocator& allocator) : allocator(allocator) {
    init_type_makers();
    init_node_makers();
}

void CTranslator::init_type_makers() {
    type_makers[ZigClangBuiltinTypeOCLImage1dRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dRO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAAWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAAWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dWO] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dRW] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMcePayload] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImePayload] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefPayload] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicPayload] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMceResult] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResult] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefResult] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicResult] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultSingleReferenceStreamout] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultDualReferenceStreamout] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeSingleReferenceStreamin] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeDualReferenceStreamin] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBool] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBoolx2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBoolx4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveCount] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVectorQuad] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVectorPair] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool1] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool32] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool64] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x5] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x6] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x7] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x3] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x4] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m4x2] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWasmExternRef] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVoid] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<VoidType>()) VoidType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeBool] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<BoolType>()) BoolType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeChar_U] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUChar] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWChar_U] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar8] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar32] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShort] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<UShortType>()) UShortType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeUInt] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<UIntType>()) UIntType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeULong] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<ULongType>()) ULongType(sizeof(long) == 8, nullptr);
    };
    type_makers[ZigClangBuiltinTypeULongLong] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeUInt128] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(nullptr);
    };
    type_makers[ZigClangBuiltinTypeChar_S] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<CharType>()) CharType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeSChar] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWChar_S] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeShort] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<ShortType>()) ShortType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeInt] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<IntType>()) IntType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeLong] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<LongType>()) LongType(sizeof(long) == 8, nullptr);
    };
    type_makers[ZigClangBuiltinTypeLongLong] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<BigIntType>()) BigIntType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeInt128] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<Int128Type>()) Int128Type(nullptr);
    };
    type_makers[ZigClangBuiltinTypeShortAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeLongAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShortAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeULongAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeShortFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeLongFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShortFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeULongFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatShortAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatLongAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUShortAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatULongAccum] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatShortFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatLongFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUShortFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatULongFract] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeHalf] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<FloatType>()) FloatType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeDouble] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return new (allocator.allocate<DoubleType>()) DoubleType(nullptr);
    };
    type_makers[ZigClangBuiltinTypeLongDouble] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBFloat16] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat128] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeIbm128] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeNullPtr] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCId] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCClass] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCSel] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLSampler] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLEvent] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLClkEvent] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLQueue] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLReserveID] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeDependent] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOverload] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBoundMember] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypePseudoObject] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUnknownAny] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBuiltinFn] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeARCUnbridgedCast] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeIncompleteMatrixIdx] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPArraySection] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPArrayShaping] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPIterator] = [](ASTAllocator& allocator, clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
}

void CTranslator::init_node_makers() {
    node_makers[ZigClangDeclAccessSpec] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclBlock] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclCaptured] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclClassScopeFunctionSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclEmpty] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclExport] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclExternCContext] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclFileScopeAsm] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclFriend] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclFriendTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclImplicitConceptSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclImport] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclLifetimeExtendedTemporary] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclLinkageSpec] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUsing] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUsingEnum] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclHLSLBuffer] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclLabel] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclNamespace] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclNamespaceAlias] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCCompatibleAlias] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCCategory] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCCategoryImpl] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCImplementation] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCInterface] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCProtocol] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCMethod] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCProperty] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclBuiltinTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclConcept] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclClassTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclFunctionTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTypeAliasTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclVarTemplate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTemplateTemplateParm] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclEnum] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_enum((clang::EnumDecl*) decl);
    };
    node_makers[ZigClangDeclRecord] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_struct((clang::RecordDecl*) decl);
    };
    node_makers[ZigClangDeclCXXRecord] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclClassTemplateSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclClassTemplatePartialSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTemplateTypeParm] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCTypeParam] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTypeAlias] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTypedef] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_typealias((clang::TypedefDecl*) decl);
    };
    node_makers[ZigClangDeclUnresolvedUsingTypename] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUnresolvedUsingIfExists] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUsingDirective] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUsingPack] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUsingShadow] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclConstructorUsingShadow] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclBinding] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclField] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCAtDefsField] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCIvar] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclFunction] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_func((clang::FunctionDecl*) decl);
    };
    node_makers[ZigClangDeclCXXDeductionGuide] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclCXXMethod] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclCXXConstructor] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclCXXConversion] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclCXXDestructor] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclMSProperty] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclNonTypeTemplateParm] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclVar] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_var_init((clang::VarDecl*) decl);
    };
    node_makers[ZigClangDeclDecomposition] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclImplicitParam] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPCapturedExpr] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclParmVar] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclVarTemplateSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclVarTemplatePartialSpecialization] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclEnumConstant] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclIndirectField] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclMSGuid] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPDeclareMapper] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPDeclareReduction] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTemplateParamObject] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUnnamedGlobalConstant] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclUnresolvedUsingValue] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPAllocate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPRequires] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclOMPThreadPrivate] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclObjCPropertyImpl] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclPragmaComment] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclPragmaDetectMismatch] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclRequiresExprBody] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclStaticAssert] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTopLevelStmt] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
    node_makers[ZigClangDeclTranslationUnit] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return nullptr;
    };
}

void CTranslator::error(const std::string& err) {
// this can be turned on if exceptions are occurring and there's output in console
//#ifdef DEBUG
//    std::cerr << "error :" << err << std::endl;
//#endif
    errors.emplace_back(
        err
    );
}