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

CTranslator::CTranslator() {
    init_type_makers();
    init_node_makers();
}

void CTranslator::init_type_makers() {
    type_makers[ZigClangBuiltinTypeOCLImage1dRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dRO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAAWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAAWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dWO] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLImage3dRW] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMcePayload] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImePayload] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefPayload] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicPayload] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMceResult] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResult] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefResult] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicResult] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultSingleReferenceStreamout] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultDualReferenceStreamout] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeSingleReferenceStreamin] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeDualReferenceStreamin] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt8x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt16x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt32x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveInt64x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint8x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint16x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint32x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveUint64x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat16x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat32x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveFloat64x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBFloat16x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBool] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBoolx2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveBoolx4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSveCount] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVectorQuad] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVectorPair] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool1] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool32] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvBool64] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt8m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint8m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt16m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint16m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt32m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint32m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvInt64m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvUint64m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat16m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat32m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x5] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x6] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x7] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x3] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x4] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeRvvFloat64m4x2] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWasmExternRef] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeVoid] = [](clang::BuiltinType*) -> BaseType* {
        return new VoidType();
    };
    type_makers[ZigClangBuiltinTypeBool] = [](clang::BuiltinType*) -> BaseType* {
        return new BoolType();
    };
    type_makers[ZigClangBuiltinTypeChar_U] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUChar] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWChar_U] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar8] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeChar32] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShort] = [](clang::BuiltinType*) -> BaseType* {
        return new UShortType();
    };
    type_makers[ZigClangBuiltinTypeUInt] = [](clang::BuiltinType*) -> BaseType* {
        return new UIntType();
    };
    type_makers[ZigClangBuiltinTypeULong] = [](clang::BuiltinType*) -> BaseType* {
        return new ULongType(sizeof(long) == 8);
    };
    type_makers[ZigClangBuiltinTypeULongLong] = [](clang::BuiltinType*) -> BaseType* {
        return new UBigIntType();
    };
    type_makers[ZigClangBuiltinTypeUInt128] = [](clang::BuiltinType*) -> BaseType* {
        return new UInt128Type();
    };
    type_makers[ZigClangBuiltinTypeChar_S] = [](clang::BuiltinType*) -> BaseType* {
        return new CharType();
    };
    type_makers[ZigClangBuiltinTypeSChar] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeWChar_S] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeShort] = [](clang::BuiltinType*) -> BaseType* {
        return new ShortType();
    };
    type_makers[ZigClangBuiltinTypeInt] = [](clang::BuiltinType*) -> BaseType* {
        return new IntType();
    };
    type_makers[ZigClangBuiltinTypeLong] = [](clang::BuiltinType*) -> BaseType* {
        return new LongType(sizeof(long) == 8);
    };
    type_makers[ZigClangBuiltinTypeLongLong] = [](clang::BuiltinType*) -> BaseType* {
        return new BigIntType();
    };
    type_makers[ZigClangBuiltinTypeInt128] = [](clang::BuiltinType*) -> BaseType* {
        return new Int128Type();
    };
    type_makers[ZigClangBuiltinTypeShortAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeLongAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShortAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeULongAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeShortFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeLongFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUShortFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeULongFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatShortAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatLongAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUShortAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatULongAccum] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatShortFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatLongFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUShortFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatUFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeSatULongFract] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeHalf] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat] = [](clang::BuiltinType*) -> BaseType* {
        return new FloatType();
    };
    type_makers[ZigClangBuiltinTypeDouble] = [](clang::BuiltinType*) -> BaseType* {
        return new DoubleType();
    };
    type_makers[ZigClangBuiltinTypeLongDouble] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBFloat16] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeFloat128] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeIbm128] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeNullPtr] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCId] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCClass] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeObjCSel] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLSampler] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLEvent] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLClkEvent] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLQueue] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOCLReserveID] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeDependent] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOverload] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBoundMember] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypePseudoObject] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeUnknownAny] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeBuiltinFn] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeARCUnbridgedCast] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeIncompleteMatrixIdx] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPArraySection] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPArrayShaping] = [](clang::BuiltinType*) -> BaseType* {
        return nullptr;
    };
    type_makers[ZigClangBuiltinTypeOMPIterator] = [](clang::BuiltinType*) -> BaseType* {
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

void CTranslator::dispatch_before() {
    for(auto& node : before_nodes) {
        nodes.emplace_back(std::move(node));
    }
    before_nodes.clear();
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