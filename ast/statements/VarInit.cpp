// Copyright (c) Chemical Language Foundation 2025.

#include "VarInit.h"
#include "ast/types/ArrayType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/values/StringValue.h"
#include "ast/values/VariableIdentifier.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/types/GenericType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/utils/ASTUtils.h"
#include "preprocess/2c/BufferedWriter.h"


BaseType* VarInitStatement::known_type() {
    if(type) {
        return type;
    }
    return value->getType();
}

