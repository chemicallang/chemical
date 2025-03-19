// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/ASTNode.h"
#include "CTranslator.h"
#include <iostream>
#include "ast/types/IntType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/LongType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/VoidType.h"
#include "ast/types/UCharType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/PointerType.h"
#include "ast/types/BoolType.h"
#include "ast/types/UShortType.h"
#include "ast/types/UIntType.h"
#include "ast/types/ULongType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/ShortType.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/Int128Type.h"

void CTranslator::init_type_makers() {
    type_makers[ZigClangBuiltinTypeOCLImage1dRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage3dRO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAAWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAAWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage3dWO] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dArrayRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage1dBufferRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dDepthRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayDepthRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAARW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAARW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dMSAADepthRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage2dArrayMSAADepthRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLImage3dRW] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMcePayload] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImePayload] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefPayload] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicPayload] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCMceResult] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResult] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCRefResult] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCSicResult] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultSingleReferenceStreamout] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeResultDualReferenceStreamout] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeSingleReferenceStreamin] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLIntelSubgroupAVCImeDualReferenceStreamin] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt8] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt16] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt32] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt64] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint8] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint16] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint32] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint64] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat16] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat32] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat64] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBFloat16] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt8x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt16x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt32x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt64x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint8x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint16x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint32x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint64x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat16x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat32x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat64x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBFloat16x2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt8x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt16x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt32x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt64x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint8x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint16x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint32x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint64x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat16x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat32x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat64x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBFloat16x3] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt8x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt16x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt32x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveInt64x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint8x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint16x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint32x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveUint64x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat16x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat32x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveFloat64x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBFloat16x4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBool] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBoolx2] = nullptr;
    type_makers[ZigClangBuiltinTypeSveBoolx4] = nullptr;
    type_makers[ZigClangBuiltinTypeSveCount] = nullptr;
    type_makers[ZigClangBuiltinTypeVectorQuad] = nullptr;
    type_makers[ZigClangBuiltinTypeVectorPair] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool1] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool16] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool32] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvBool64] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf8x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf4x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt8m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf8x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf4x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint8m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf4x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt16m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf4x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint16m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt32m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint32m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvInt64m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvUint64m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf4x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat16m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32mf2x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat32m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x5] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x6] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x7] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m1x8] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x2] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x3] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m2x4] = nullptr;
    type_makers[ZigClangBuiltinTypeRvvFloat64m4x2] = nullptr;
    type_makers[ZigClangBuiltinTypeWasmExternRef] = nullptr;
    type_makers[ZigClangBuiltinTypeVoid] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<VoidType>()) VoidType(location);
    };
    type_makers[ZigClangBuiltinTypeBool] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<BoolType>()) BoolType(location);
    };
    type_makers[ZigClangBuiltinTypeChar_U] = nullptr;
    type_makers[ZigClangBuiltinTypeUChar] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<UCharType>()) UCharType(location);
    };;
    type_makers[ZigClangBuiltinTypeWChar_U] = nullptr;
    type_makers[ZigClangBuiltinTypeChar8] = nullptr;
    type_makers[ZigClangBuiltinTypeChar16] = nullptr;
    type_makers[ZigClangBuiltinTypeChar32] = nullptr;
    type_makers[ZigClangBuiltinTypeUShort] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<UShortType>()) UShortType(location);
    };
    type_makers[ZigClangBuiltinTypeUInt] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<UIntType>()) UIntType(location);
    };
    type_makers[ZigClangBuiltinTypeULong] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<ULongType>()) ULongType(location);
    };
    type_makers[ZigClangBuiltinTypeULongLong] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(location);
    };
    type_makers[ZigClangBuiltinTypeUInt128] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(location);
    };
    type_makers[ZigClangBuiltinTypeChar_S] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<CharType>()) CharType(location);
    };
    type_makers[ZigClangBuiltinTypeSChar] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<CharType>()) CharType(location);
    };
    type_makers[ZigClangBuiltinTypeWChar_S] = nullptr;
    type_makers[ZigClangBuiltinTypeShort] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<ShortType>()) ShortType(location);
    };
    type_makers[ZigClangBuiltinTypeInt] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<IntType>()) IntType(location);
    };
    type_makers[ZigClangBuiltinTypeLong] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<LongType>()) LongType(location);
    };
    type_makers[ZigClangBuiltinTypeLongLong] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<BigIntType>()) BigIntType(location);
    };
    type_makers[ZigClangBuiltinTypeInt128] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<Int128Type>()) Int128Type(location);
    };
    type_makers[ZigClangBuiltinTypeShortAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeLongAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeUShortAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeUAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeULongAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeShortFract] = nullptr;
    type_makers[ZigClangBuiltinTypeFract] = nullptr;
    type_makers[ZigClangBuiltinTypeLongFract] = nullptr;
    type_makers[ZigClangBuiltinTypeUShortFract] = nullptr;
    type_makers[ZigClangBuiltinTypeUFract] = nullptr;
    type_makers[ZigClangBuiltinTypeULongFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatShortAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatLongAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatUShortAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatUAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatULongAccum] = nullptr;
    type_makers[ZigClangBuiltinTypeSatShortFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatLongFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatUShortFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatUFract] = nullptr;
    type_makers[ZigClangBuiltinTypeSatULongFract] = nullptr;
    type_makers[ZigClangBuiltinTypeHalf] = nullptr;
    type_makers[ZigClangBuiltinTypeFloat] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<FloatType>()) FloatType(location);
    };
    type_makers[ZigClangBuiltinTypeDouble] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<DoubleType>()) DoubleType(location);
    };
    type_makers[ZigClangBuiltinTypeLongDouble] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<LongDoubleType>()) LongDoubleType(location);
    };
    type_makers[ZigClangBuiltinTypeFloat16] = nullptr;
    type_makers[ZigClangBuiltinTypeBFloat16] = nullptr;
    type_makers[ZigClangBuiltinTypeFloat128] = [](ASTAllocator& allocator, clang::BuiltinType*, SourceLocation location) -> BaseType* {
        return new (allocator.allocate<Float128Type>()) Float128Type(location);
    };
    type_makers[ZigClangBuiltinTypeIbm128] = nullptr;
    type_makers[ZigClangBuiltinTypeNullPtr] = nullptr;
    type_makers[ZigClangBuiltinTypeObjCId] = nullptr;
    type_makers[ZigClangBuiltinTypeObjCClass] = nullptr;
    type_makers[ZigClangBuiltinTypeObjCSel] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLSampler] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLEvent] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLClkEvent] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLQueue] = nullptr;
    type_makers[ZigClangBuiltinTypeOCLReserveID] = nullptr;
    type_makers[ZigClangBuiltinTypeDependent] = nullptr;
    type_makers[ZigClangBuiltinTypeOverload] = nullptr;
    type_makers[ZigClangBuiltinTypeBoundMember] = nullptr;
    type_makers[ZigClangBuiltinTypePseudoObject] = nullptr;
    type_makers[ZigClangBuiltinTypeUnknownAny] = nullptr;
    type_makers[ZigClangBuiltinTypeBuiltinFn] = nullptr;
    type_makers[ZigClangBuiltinTypeARCUnbridgedCast] = nullptr;
    type_makers[ZigClangBuiltinTypeIncompleteMatrixIdx] = nullptr;
    type_makers[ZigClangBuiltinTypeOMPArraySection] = nullptr;
    type_makers[ZigClangBuiltinTypeOMPArrayShaping] = nullptr;
    type_makers[ZigClangBuiltinTypeOMPIterator] = nullptr;
}

void CTranslator::init_node_makers() {
    node_makers[ZigClangDeclAccessSpec] = nullptr;
    node_makers[ZigClangDeclBlock] = nullptr;
    node_makers[ZigClangDeclCaptured] = nullptr;
    node_makers[ZigClangDeclClassScopeFunctionSpecialization] = nullptr;
    node_makers[ZigClangDeclEmpty] = nullptr;
    node_makers[ZigClangDeclExport] = nullptr;
    node_makers[ZigClangDeclExternCContext] = nullptr;
    node_makers[ZigClangDeclFileScopeAsm] = nullptr;
    node_makers[ZigClangDeclFriend] = nullptr;
    node_makers[ZigClangDeclFriendTemplate] = nullptr;
    node_makers[ZigClangDeclImplicitConceptSpecialization] = nullptr;
    node_makers[ZigClangDeclImport] = nullptr;
    node_makers[ZigClangDeclLifetimeExtendedTemporary] = nullptr;
    node_makers[ZigClangDeclLinkageSpec] = nullptr;
    node_makers[ZigClangDeclUsing] = nullptr;
    node_makers[ZigClangDeclUsingEnum] = nullptr;
    node_makers[ZigClangDeclHLSLBuffer] = nullptr;
    node_makers[ZigClangDeclLabel] = nullptr;
    node_makers[ZigClangDeclNamespace] = nullptr;
    node_makers[ZigClangDeclNamespaceAlias] = nullptr;
    node_makers[ZigClangDeclObjCCompatibleAlias] = nullptr;
    node_makers[ZigClangDeclObjCCategory] = nullptr;
    node_makers[ZigClangDeclObjCCategoryImpl] = nullptr;
    node_makers[ZigClangDeclObjCImplementation] = nullptr;
    node_makers[ZigClangDeclObjCInterface] = nullptr;
    node_makers[ZigClangDeclObjCProtocol] = nullptr;
    node_makers[ZigClangDeclObjCMethod] = nullptr;
    node_makers[ZigClangDeclObjCProperty] = nullptr;
    node_makers[ZigClangDeclBuiltinTemplate] = nullptr;
    node_makers[ZigClangDeclConcept] = nullptr;
    node_makers[ZigClangDeclClassTemplate] = nullptr;
    node_makers[ZigClangDeclFunctionTemplate] = nullptr;
    node_makers[ZigClangDeclTypeAliasTemplate] = nullptr;
    node_makers[ZigClangDeclVarTemplate] = nullptr;
    node_makers[ZigClangDeclTemplateTemplateParm] = nullptr;
    node_makers[ZigClangDeclEnum] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_enum((clang::EnumDecl*) decl);
    };
    node_makers[ZigClangDeclRecord] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_struct((clang::RecordDecl*) decl);
    };
    node_makers[ZigClangDeclCXXRecord] = nullptr;
    node_makers[ZigClangDeclClassTemplateSpecialization] = nullptr;
    node_makers[ZigClangDeclClassTemplatePartialSpecialization] = nullptr;
    node_makers[ZigClangDeclTemplateTypeParm] = nullptr;
    node_makers[ZigClangDeclObjCTypeParam] = nullptr;
    node_makers[ZigClangDeclTypeAlias] = nullptr;
    node_makers[ZigClangDeclTypedef] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_typealias((clang::TypedefDecl*) decl);
    };
    node_makers[ZigClangDeclUnresolvedUsingTypename] = nullptr;
    node_makers[ZigClangDeclUnresolvedUsingIfExists] = nullptr;
    node_makers[ZigClangDeclUsingDirective] = nullptr;
    node_makers[ZigClangDeclUsingPack] = nullptr;
    node_makers[ZigClangDeclUsingShadow] = nullptr;
    node_makers[ZigClangDeclConstructorUsingShadow] = nullptr;
    node_makers[ZigClangDeclBinding] = nullptr;
    node_makers[ZigClangDeclField] = nullptr;
    node_makers[ZigClangDeclObjCAtDefsField] = nullptr;
    node_makers[ZigClangDeclObjCIvar] = nullptr;
    node_makers[ZigClangDeclFunction] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_func((clang::FunctionDecl*) decl);
    };
    node_makers[ZigClangDeclCXXDeductionGuide] = nullptr;
    node_makers[ZigClangDeclCXXMethod] = nullptr;
    node_makers[ZigClangDeclCXXConstructor] = nullptr;
    node_makers[ZigClangDeclCXXConversion] = nullptr;
    node_makers[ZigClangDeclCXXDestructor] = nullptr;
    node_makers[ZigClangDeclMSProperty] = nullptr;
    node_makers[ZigClangDeclNonTypeTemplateParm] = nullptr;
    node_makers[ZigClangDeclVar] = [](CTranslator* translator, clang::Decl* decl) -> ASTNode* {
        return (ASTNode*) translator->make_var_init((clang::VarDecl*) decl);
    };
    node_makers[ZigClangDeclDecomposition] = nullptr;
    node_makers[ZigClangDeclImplicitParam] = nullptr;
    node_makers[ZigClangDeclOMPCapturedExpr] = nullptr;
    node_makers[ZigClangDeclParmVar] = nullptr;
    node_makers[ZigClangDeclVarTemplateSpecialization] = nullptr;
    node_makers[ZigClangDeclVarTemplatePartialSpecialization] = nullptr;
    node_makers[ZigClangDeclEnumConstant] = nullptr;
    node_makers[ZigClangDeclIndirectField] = nullptr;
    node_makers[ZigClangDeclMSGuid] = nullptr;
    node_makers[ZigClangDeclOMPDeclareMapper] = nullptr;
    node_makers[ZigClangDeclOMPDeclareReduction] = nullptr;
    node_makers[ZigClangDeclTemplateParamObject] = nullptr;
    node_makers[ZigClangDeclUnnamedGlobalConstant] = nullptr;
    node_makers[ZigClangDeclUnresolvedUsingValue] = nullptr;
    node_makers[ZigClangDeclOMPAllocate] = nullptr;
    node_makers[ZigClangDeclOMPRequires] = nullptr;
    node_makers[ZigClangDeclOMPThreadPrivate] = nullptr;
    node_makers[ZigClangDeclObjCPropertyImpl] = nullptr;
    node_makers[ZigClangDeclPragmaComment] = nullptr;
    node_makers[ZigClangDeclPragmaDetectMismatch] = nullptr;
    node_makers[ZigClangDeclRequiresExprBody] = nullptr;
    node_makers[ZigClangDeclStaticAssert] = nullptr;
    node_makers[ZigClangDeclTopLevelStmt] = nullptr;
    node_makers[ZigClangDeclTranslationUnit] = nullptr;
}

void CTranslator::module_begin() {
    declared_in_module.clear();
}

void CTranslator::error(const std::string& err) {
// this can be turned on if exceptions are occurring and there's no output in console
#ifdef DEBUG
    std::cerr << "[CTranslator] Debug error :" << err << std::endl;
#endif
    diagnostics.emplace_back(
        Range { Position { 0, 0 }, Position { 0, 0 } },
        DiagSeverity::Error,
        std::nullopt,
        err
    );
}