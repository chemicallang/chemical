// Copyright (c) Qinetik 2024.

#include "SizeOfValue.h"
#include "AlignOfValue.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ULongValue.h"

bool SizeOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type->link(linker);
    return true;
}

Value* SizeOfValue::evaluated_value(InterpretScope &scope) {
    return new (scope.allocate<UBigIntValue>()) UBigIntValue(for_type->byte_size(scope.global->target_data.is_64Bit), encoded_location());
}

bool AlignOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type->link(linker);
    return true;
}

Value* AlignOfValue::evaluated_value(InterpretScope &scope) {
    return new (scope.allocate<UBigIntValue>()) UBigIntValue(for_type->type_alignment(scope.global->target_data.is_64Bit), encoded_location());
}