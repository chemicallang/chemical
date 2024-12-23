// Copyright (c) Qinetik 2024.

#include "SizeOfValue.h"
#include "AlignOfValue.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ULongValue.h"

SizeOfValue::SizeOfValue(BaseType* for_type, SourceLocation location) : for_type(for_type), location(location) {

}

bool SizeOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type->link(linker);
    return true;
}

AlignOfValue::AlignOfValue(BaseType* for_type, SourceLocation location) : for_type(for_type), location(location) {

}

bool AlignOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type->link(linker);
    return true;
}