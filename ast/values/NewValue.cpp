// Copyright (c) Chemical Language Foundation 2025.

#include "NewValue.h"
#include "PlacementNewValue.h"
#include "NewTypedValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/PointerType.h"
#include "ast/types/IntNType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/structures/VariantDefinition.h"
#include <cstring>

