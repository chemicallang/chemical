// Copyright (c) Chemical Language Foundation 2025.

#include "NewValue.h"
#include "PlacementNewValue.h"
#include "NewTypedValue.h"
#include "ast/types/PointerType.h"

BaseType* NewTypedValue::known_type() {
    ptr_type.type = type;
    return &ptr_type;
}

BaseType* NewValue::known_type() {
    auto type = value->known_type();
    ptr_type.type = type;
    return &ptr_type;
}

BaseType* PlacementNewValue::known_type() {
    auto type = value->known_type();
    ptr_type.type = type;
    return &ptr_type;
}